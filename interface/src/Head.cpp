//
//  Head.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <glm/gtx/quaternion.hpp>

#include <QImage>

#include <NodeList.h>

#include "Application.h"
#include "Avatar.h"
#include "Head.h"
#include "Util.h"
#include "renderer/ProgramObject.h"

using namespace std;
 
const int   MOHAWK_TRIANGLES         =  50;
const bool  USING_PHYSICAL_MOHAWK    =  true;
const float EYE_RIGHT_OFFSET         =  0.27f;
const float EYE_UP_OFFSET            =  0.36f;
const float EYE_FRONT_OFFSET         =  0.8f;
const float EAR_RIGHT_OFFSET         =  1.0;
const float MOUTH_UP_OFFSET          = -0.3f;
const float HEAD_MOTION_DECAY        =  0.1;
const float MINIMUM_EYE_ROTATION_DOT =  0.5f; // based on a dot product: 1.0 is straight ahead, 0.0 is 90 degrees off
const float EYEBALL_RADIUS           =  0.017;
const float EYELID_RADIUS            =  0.019; 
const float EYEBALL_COLOR[3]         =  { 0.9f, 0.9f, 0.8f };
const float HAIR_SPRING_FORCE        =  10.0f;
const float HAIR_TORQUE_FORCE        =  0.1f;
const float HAIR_GRAVITY_FORCE       =  0.05f;
const float HAIR_DRAG                =  10.0f;
const float HAIR_LENGTH              =  0.09f;
const float HAIR_THICKNESS           =  0.03f;
const float IRIS_RADIUS              =  0.007;
const float IRIS_PROTRUSION          =  0.0145f;
const char  IRIS_TEXTURE_FILENAME[]  =  "resources/images/iris.png";

ProgramObject* Head::_irisProgram = 0;
GLuint Head::_irisTextureID;
int Head::_eyePositionLocation;

Head::Head(Avatar* owningAvatar) :
    HeadData((AvatarData*)owningAvatar),
    yawRate(0.0f),
    _renderAlpha(0.0),
    _returnHeadToCenter(false),
    _skinColor(0.0f, 0.0f, 0.0f),
    _position(0.0f, 0.0f, 0.0f),
    _rotation(0.0f, 0.0f, 0.0f),
    _leftEyePosition(0.0f, 0.0f, 0.0f),
    _rightEyePosition(0.0f, 0.0f, 0.0f),
    _eyeLevelPosition(0.0f, 0.0f, 0.0f),
    _leftEyeBrowPosition(0.0f, 0.0f, 0.0f),
    _rightEyeBrowPosition(0.0f, 0.0f, 0.0f),
    _leftEarPosition(0.0f, 0.0f, 0.0f),
    _rightEarPosition(0.0f, 0.0f, 0.0f),
    _mouthPosition(0.0f, 0.0f, 0.0f),
    _scale(1.0f),
    _browAudioLift(0.0f),
    _gravity(0.0f, -1.0f, 0.0f),
    _lastLoudness(0.0f),
    _averageLoudness(0.0f),
    _audioAttack(0.0f),
    _returnSpringScale(1.0f),
    _bodyRotation(0.0f, 0.0f, 0.0f),
    _renderLookatVectors(false),
    _mohawkTriangleFan(NULL),
     _mohawkColors(NULL),
    _saccade(0.0f, 0.0f, 0.0f),
    _saccadeTarget(0.0f, 0.0f, 0.0f),
    _leftEyeBlink(0.0f),
    _rightEyeBlink(0.0f),
    _leftEyeBlinkVelocity(0.0f),
    _rightEyeBlinkVelocity(0.0f),
    _timeWithoutTalking(0.0f),
    _cameraPitch(_pitch),
    _cameraYaw(_yaw),
    _isCameraMoving(false),
    _cameraFollowsHead(false),
    _cameraFollowHeadRate(0.0f)
{
    if (USING_PHYSICAL_MOHAWK) {
        resetHairPhysics();
    }
}

void Head::init() {
    if (_irisProgram == 0) {
        switchToResourcesParentIfRequired();
        _irisProgram = new ProgramObject();
        _irisProgram->addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/iris.vert");
        _irisProgram->addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/iris.frag");
        _irisProgram->link();
    
        _irisProgram->setUniformValue("texture", 0);
        _eyePositionLocation = _irisProgram->uniformLocation("eyePosition");
        
        QImage image = QImage(IRIS_TEXTURE_FILENAME).convertToFormat(QImage::Format_ARGB32);
        
        glGenTextures(1, &_irisTextureID);
        glBindTexture(GL_TEXTURE_2D, _irisTextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 1, GL_BGRA, GL_UNSIGNED_BYTE, image.constBits());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Head::reset() {
    _yaw = _pitch = _roll = 0.0f;
    _leanForward = _leanSideways = 0.0f;
    
    if (USING_PHYSICAL_MOHAWK) {
        resetHairPhysics();
    }
}




void Head::resetHairPhysics() {
    glm::vec3 up = getUpDirection();
    for (int t = 0; t < NUM_HAIR_TUFTS; t ++) {
        
        _hairTuft[t].length       = HAIR_LENGTH;
        _hairTuft[t].thickness    = HAIR_THICKNESS;
        _hairTuft[t].basePosition = _position + up * _scale * 0.9f;
        _hairTuft[t].midPosition  = _hairTuft[t].basePosition + up * _hairTuft[t].length * ONE_HALF;			
        _hairTuft[t].endPosition  = _hairTuft[t].midPosition  + up * _hairTuft[t].length * ONE_HALF;			
        _hairTuft[t].midVelocity  = glm::vec3(0.0f, 0.0f, 0.0f);			
        _hairTuft[t].endVelocity  = glm::vec3(0.0f, 0.0f, 0.0f);	
    }
}


void Head::simulate(float deltaTime, bool isMine) {
    
    // Update eye saccades
    const float AVERAGE_MICROSACCADE_INTERVAL = 0.50f;
    const float AVERAGE_SACCADE_INTERVAL = 4.0f;
    const float MICROSACCADE_MAGNITUDE = 0.002f;
    const float SACCADE_MAGNITUDE = 0.04;
    
    if (randFloat() < deltaTime / AVERAGE_MICROSACCADE_INTERVAL) {
        _saccadeTarget = MICROSACCADE_MAGNITUDE * randVector();
    } else if (randFloat() < deltaTime / AVERAGE_SACCADE_INTERVAL) {
        _saccadeTarget = SACCADE_MAGNITUDE * randVector();
    }
    _saccade += (_saccadeTarget - _saccade) * 0.50f;
    
    //  Update audio trailing average for rendering facial animations
    const float AUDIO_AVERAGING_SECS = 0.05;
    _averageLoudness = (1.f - deltaTime / AUDIO_AVERAGING_SECS) * _averageLoudness +
                             (deltaTime / AUDIO_AVERAGING_SECS) * _audioLoudness;
    
    //  Detect transition from talking to not; force blink after that and a delay
    bool forceBlink = false;
    const float TALKING_LOUDNESS = 100.0f;
    const float BLINK_AFTER_TALKING = 0.25f;
    if (_averageLoudness > TALKING_LOUDNESS) {
        _timeWithoutTalking = 0.0f;
    
    } else if (_timeWithoutTalking < BLINK_AFTER_TALKING && (_timeWithoutTalking += deltaTime) >= BLINK_AFTER_TALKING) {
        forceBlink = true;
    }
                             
    //  Update audio attack data for facial animation (eyebrows and mouth)
    _audioAttack = 0.9 * _audioAttack + 0.1 * fabs(_audioLoudness - _lastLoudness);
    _lastLoudness = _audioLoudness;
    
    const float BROW_LIFT_THRESHOLD = 100;
    if (_audioAttack > BROW_LIFT_THRESHOLD)
        _browAudioLift += sqrt(_audioAttack) * 0.00005;
        
        float clamp = 0.01;
        if (_browAudioLift > clamp) { _browAudioLift = clamp; }
    
    _browAudioLift *= 0.7f;      

    // update eyelid blinking
    const float BLINK_SPEED = 10.0f;
    const float FULLY_OPEN = 0.0f;
    const float FULLY_CLOSED = 1.0f;
    if (_leftEyeBlinkVelocity == 0.0f && _rightEyeBlinkVelocity == 0.0f) {
        // no blinking when brows are raised; blink less with increasing loudness
        const float BASE_BLINK_RATE = 15.0f / 60.0f;
        const float ROOT_LOUDNESS_TO_BLINK_INTERVAL = 0.25f;
        if (forceBlink || (_browAudioLift < EPSILON && shouldDo(glm::max(1.0f, sqrt(_averageLoudness) *
                ROOT_LOUDNESS_TO_BLINK_INTERVAL) / BASE_BLINK_RATE, deltaTime))) {
            _leftEyeBlinkVelocity = BLINK_SPEED;
            _rightEyeBlinkVelocity = BLINK_SPEED;
        }
    } else {
        _leftEyeBlink = glm::clamp(_leftEyeBlink + _leftEyeBlinkVelocity * deltaTime, FULLY_OPEN, FULLY_CLOSED);
        _rightEyeBlink = glm::clamp(_rightEyeBlink + _rightEyeBlinkVelocity * deltaTime, FULLY_OPEN, FULLY_CLOSED);
        
        if (_leftEyeBlink == FULLY_CLOSED) {
            _leftEyeBlinkVelocity = -BLINK_SPEED;
        
        } else if (_leftEyeBlink == FULLY_OPEN) {
            _leftEyeBlinkVelocity = 0.0f;
        }
        if (_rightEyeBlink == FULLY_CLOSED) {
            _rightEyeBlinkVelocity = -BLINK_SPEED;
        
        } else if (_rightEyeBlink == FULLY_OPEN) {
            _rightEyeBlinkVelocity = 0.0f;
        }
    }

    // based on the nature of the lookat position, determine if the eyes can look / are looking at it.      
    if (USING_PHYSICAL_MOHAWK) {
        updateHairPhysics(deltaTime);
    }
    
    // Update camera pitch and yaw independently from motion of head (for gyro-based interface)
    if (isMine && _cameraFollowsHead) {
        //  If we are using gyros and using gyroLook, have the camera follow head but with a null region
        //  to create stable rendering view with small head movements.
        const float CAMERA_FOLLOW_HEAD_RATE_START = 0.01f;
        const float CAMERA_FOLLOW_HEAD_RATE_MAX = 0.5f;
        const float CAMERA_FOLLOW_HEAD_RATE_RAMP_RATE = 1.05f;
        const float CAMERA_STOP_TOLERANCE_DEGREES = 0.1f;
        const float CAMERA_START_TOLERANCE_DEGREES = 2.0f;
        float cameraHeadAngleDifference = glm::length(glm::vec2(_pitch - _cameraPitch, _yaw - _cameraYaw));
        if (_isCameraMoving) {
            _cameraFollowHeadRate = glm::clamp(_cameraFollowHeadRate * CAMERA_FOLLOW_HEAD_RATE_RAMP_RATE,
                                               0.f,
                                               CAMERA_FOLLOW_HEAD_RATE_MAX);
                                               
            _cameraPitch += (_pitch - _cameraPitch) * _cameraFollowHeadRate;
            _cameraYaw += (_yaw - _cameraYaw) * _cameraFollowHeadRate;
            if (cameraHeadAngleDifference < CAMERA_STOP_TOLERANCE_DEGREES) {
                _isCameraMoving = false;
            }
        } else {
            if (cameraHeadAngleDifference > CAMERA_START_TOLERANCE_DEGREES) {
                _isCameraMoving = true;
                _cameraFollowHeadRate = CAMERA_FOLLOW_HEAD_RATE_START;
            }
        }
    } else {
        //  Camera always locked to head 
        _cameraPitch = _pitch;
        _cameraYaw = _yaw;
    }
}

void Head::calculateGeometry() {
    //generate orientation directions 
    glm::quat orientation = getOrientation();
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    glm::vec3 up    = orientation * IDENTITY_UP;
    glm::vec3 front = orientation * IDENTITY_FRONT;

    //calculate the eye positions 
    _leftEyePosition  = _position 
                      - right * _scale * EYE_RIGHT_OFFSET
                      + up * _scale * EYE_UP_OFFSET
                      + front * _scale * EYE_FRONT_OFFSET;
    _rightEyePosition = _position
                      + right * _scale * EYE_RIGHT_OFFSET 
                      + up * _scale * EYE_UP_OFFSET 
                      + front * _scale * EYE_FRONT_OFFSET;

    _eyeLevelPosition = _position + up * _scale * EYE_UP_OFFSET;

    //calculate the eyebrow positions 
    _leftEyeBrowPosition  = _leftEyePosition; 
    _rightEyeBrowPosition = _rightEyePosition;
    
    //calculate the ear positions 
    _leftEarPosition  = _position - right * _scale * EAR_RIGHT_OFFSET;
    _rightEarPosition = _position + right * _scale * EAR_RIGHT_OFFSET;

    //calculate the mouth position 
    _mouthPosition    = _position + up * _scale * MOUTH_UP_OFFSET
                                  + front * _scale;
}


void Head::render(float alpha) {

    _renderAlpha = alpha;

    calculateGeometry();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_RESCALE_NORMAL);
    
    renderMohawk();
    renderHeadSphere();
    renderEyeBalls();    
    renderEars();
    renderMouth();    
    renderEyeBrows();
        
    if (_renderLookatVectors) {
        renderLookatVectors(_leftEyePosition, _rightEyePosition, _lookAtPosition);
    }
}


void Head::createMohawk() {
    uint16_t nodeId = 0;
    if (_owningAvatar->getOwningNode()) {
        nodeId = _owningAvatar->getOwningNode()->getNodeID();
    } else {
        nodeId = NodeList::getInstance()->getOwnerID();
        if (nodeId == UNKNOWN_NODE_ID) {
            return;
        }
    }
    srand(nodeId);
    float height = 0.08f + randFloat() * 0.05f;
    float variance = 0.03 + randFloat() * 0.03f;
    const float RAD_PER_TRIANGLE = (2.3f + randFloat() * 0.2f) / (float)MOHAWK_TRIANGLES;
    _mohawkTriangleFan = new glm::vec3[MOHAWK_TRIANGLES];
    _mohawkColors = new glm::vec3[MOHAWK_TRIANGLES];
    _mohawkTriangleFan[0] = glm::vec3(0, 0, 0);
    glm::vec3 basicColor(randFloat(), randFloat(), randFloat());
    _mohawkColors[0] = basicColor;
        
    for (int i = 1; i < MOHAWK_TRIANGLES; i++) {
        _mohawkTriangleFan[i]  = glm::vec3((randFloat() - 0.5f) * variance,
                                           height * cosf(i * RAD_PER_TRIANGLE - PIf / 2.f)
                                           + (randFloat()  - 0.5f) * variance,
                                           height * sinf(i * RAD_PER_TRIANGLE - PIf / 2.f)
                                           + (randFloat() - 0.5f) * variance);
        _mohawkColors[i] = randFloat() * basicColor;

    }
}

void Head::renderMohawk() {
    
    if (!_mohawkTriangleFan) {
        createMohawk();
    }
    
    if (USING_PHYSICAL_MOHAWK) {
        for (int t = 0; t < NUM_HAIR_TUFTS; t ++) {

            glm::vec3 baseAxis   = _hairTuft[t].midPosition - _hairTuft[t].basePosition;
            glm::vec3 midAxis    = _hairTuft[t].endPosition - _hairTuft[t].midPosition;
            glm::vec3 viewVector = _hairTuft[t].basePosition - Application::getInstance()->getCamera()->getPosition();
            
            glm::vec3 basePerpendicular = glm::normalize(glm::cross(baseAxis, viewVector));
            glm::vec3 midPerpendicular  = glm::normalize(glm::cross(midAxis,  viewVector));

            glm::vec3 base1 = _hairTuft[t].basePosition - basePerpendicular * _hairTuft[t].thickness * ONE_HALF;
            glm::vec3 base2 = _hairTuft[t].basePosition + basePerpendicular * _hairTuft[t].thickness * ONE_HALF;
            glm::vec3 mid1  = _hairTuft[t].midPosition  - midPerpendicular  * _hairTuft[t].thickness * ONE_HALF * ONE_HALF;
            glm::vec3 mid2  = _hairTuft[t].midPosition  + midPerpendicular  * _hairTuft[t].thickness * ONE_HALF * ONE_HALF;
            
            glColor3f(_mohawkColors[t].x, _mohawkColors[t].y, _mohawkColors[t].z);

            glBegin(GL_TRIANGLES);             
            glVertex3f(base1.x,  base1.y,  base1.z ); 
            glVertex3f(base2.x,  base2.y,  base2.z ); 
            glVertex3f(mid1.x,   mid1.y,   mid1.z  ); 
            glVertex3f(base2.x,  base2.y,  base2.z ); 
            glVertex3f(mid1.x,   mid1.y,   mid1.z  ); 
            glVertex3f(mid2.x,   mid2.y,   mid2.z  ); 
            glVertex3f(mid1.x,   mid1.y,   mid1.z  ); 
            glVertex3f(mid2.x,   mid2.y,   mid2.z  ); 
            glVertex3f(_hairTuft[t].endPosition.x, _hairTuft[t].endPosition.y, _hairTuft[t].endPosition.z  ); 
            glEnd();
        }
    } else {
        glPushMatrix();
        glTranslatef(_position.x, _position.y, _position.z);
        glRotatef(_bodyRotation.y + _yaw, 0, 1, 0);
        glRotatef(-_roll, 0, 0, 1);
        glRotatef(-_pitch - _bodyRotation.x, 1, 0, 0);
       
        glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i < MOHAWK_TRIANGLES; i++) {
            glColor3f(_mohawkColors[i].x, _mohawkColors[i].y, _mohawkColors[i].z);
            glVertex3fv(&_mohawkTriangleFan[i].x);
            glNormal3fv(&_mohawkColors[i].x);
        }
        glEnd();
        glPopMatrix();
    } 
}

glm::quat Head::getOrientation() const {
    return glm::quat(glm::radians(_bodyRotation)) * glm::quat(glm::radians(glm::vec3(_pitch, _yaw, _roll)));
}

glm::quat Head::getCameraOrientation () const {
    Avatar* owningAvatar = static_cast<Avatar*>(_owningAvatar);
    return owningAvatar->getWorldAlignedOrientation()
            * glm::quat(glm::radians(glm::vec3(_cameraPitch, _cameraYaw, 0.0f)));
}

void Head::renderHeadSphere() {
    glPushMatrix();
        glTranslatef(_position.x, _position.y, _position.z); //translate to head position
        glScalef(_scale, _scale, _scale); //scale to head size        
        glColor4f(_skinColor.x, _skinColor.y, _skinColor.z, _renderAlpha);
        glutSolidSphere(1, 30, 30);
    glPopMatrix();
}

void Head::renderEars() {

    glPushMatrix();
        glColor4f(_skinColor.x, _skinColor.y, _skinColor.z, _renderAlpha);
        glTranslatef(_leftEarPosition.x, _leftEarPosition.y, _leftEarPosition.z);        
        glutSolidSphere(0.02, 30, 30);
    glPopMatrix();

    glPushMatrix();
        glColor4f(_skinColor.x, _skinColor.y, _skinColor.z, _renderAlpha);
        glTranslatef(_rightEarPosition.x, _rightEarPosition.y, _rightEarPosition.z);        
        glutSolidSphere(0.02, 30, 30);
    glPopMatrix();
}

void Head::renderMouth() {

    float s = sqrt(_averageLoudness);

    glm::quat orientation = getOrientation();
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    glm::vec3 up    = orientation * IDENTITY_UP;
    glm::vec3 front = orientation * IDENTITY_FRONT;

    glm::vec3 r = right * _scale * (0.30f + s * 0.0014f );
    glm::vec3 u = up * _scale * (0.05f + s * 0.0040f );
    glm::vec3 f = front * _scale *  0.09f;

    glm::vec3 middle      = _mouthPosition;
    glm::vec3 leftCorner  = _mouthPosition - r * 1.0f;
    glm::vec3 rightCorner = _mouthPosition + r * 1.0f;
    glm::vec3 leftTop     = _mouthPosition - r * 0.4f + u * 0.7f + f;
    glm::vec3 rightTop    = _mouthPosition + r * 0.4f + u * 0.7f + f;
    glm::vec3 leftBottom  = _mouthPosition - r * 0.4f - u * 1.0f + f * 0.7f;
    glm::vec3 rightBottom = _mouthPosition + r * 0.4f - u * 1.0f + f * 0.7f;
    
    // constrain all mouth vertices to a sphere slightly larger than the head...
    const float MOUTH_OFFSET_OFF_FACE = 0.003f;
    
    float constrainedRadius = _scale + MOUTH_OFFSET_OFF_FACE;
    middle      = _position + glm::normalize(middle      - _position) * constrainedRadius;
    leftCorner  = _position + glm::normalize(leftCorner  - _position) * constrainedRadius;
    rightCorner = _position + glm::normalize(rightCorner - _position) * constrainedRadius;
    leftTop     = _position + glm::normalize(leftTop     - _position) * constrainedRadius;
    rightTop    = _position + glm::normalize(rightTop    - _position) * constrainedRadius;
    leftBottom  = _position + glm::normalize(leftBottom  - _position) * constrainedRadius;
    rightBottom = _position + glm::normalize(rightBottom - _position) * constrainedRadius;

    glColor3f(0.2f, 0.0f, 0.0f);
    
    glBegin(GL_TRIANGLES);             
    glVertex3f(leftCorner.x,  leftCorner.y,  leftCorner.z ); 
    glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
    glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
    glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
    glVertex3f(middle.x,      middle.y,      middle.z     ); 
    glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
    glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
    glVertex3f(middle.x,      middle.y,      middle.z     ); 
    glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
    glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
    glVertex3f(middle.x,      middle.y,      middle.z     ); 
    glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 
    glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
    glVertex3f(middle.x,      middle.y,      middle.z     ); 
    glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 
    glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
    glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 
    glVertex3f(rightCorner.x, rightCorner.y, rightCorner.z); 
    glEnd();
}

void Head::renderEyeBrows() {   

    float height = _scale * 0.3f + _browAudioLift;
    float length = _scale * 0.2f;
    float width  = _scale * 0.07f;

    glColor3f(0.3f, 0.25f, 0.2f);

    glm::vec3 leftCorner  = _leftEyePosition;
    glm::vec3 rightCorner = _leftEyePosition;
    glm::vec3 leftTop     = _leftEyePosition;
    glm::vec3 rightTop    = _leftEyePosition;
    glm::vec3 leftBottom  = _leftEyePosition;
    glm::vec3 rightBottom = _leftEyePosition;
   
    glm::quat orientation = getOrientation();
    glm::vec3 right = orientation * IDENTITY_RIGHT;
    glm::vec3 up    = orientation * IDENTITY_UP;
    glm::vec3 front = orientation * IDENTITY_FRONT;
    
    glm::vec3 r = right * length; 
    glm::vec3 u = up * height; 
    glm::vec3 t = up * (height + width); 
    glm::vec3 f = front * _scale * -0.1f;
     
    for (int i = 0; i < 2; i++) {
    
        if ( i == 1 ) {
            leftCorner = rightCorner = leftTop = rightTop = leftBottom = rightBottom = _rightEyePosition;
        }
       
        leftCorner  -= r * 1.0f;
        rightCorner += r * 1.0f;
        leftTop     -= r * 0.4f;
        rightTop    += r * 0.4f;
        leftBottom  -= r * 0.4f;
        rightBottom += r * 0.4f;

        leftCorner  += u + f;
        rightCorner += u + f;
        leftTop     += t + f;
        rightTop    += t + f;
        leftBottom  += u + f;
        rightBottom += u + f;        
        
        glBegin(GL_TRIANGLES);             

        glVertex3f(leftCorner.x,  leftCorner.y,  leftCorner.z ); 
        glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
        glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
        glVertex3f(leftTop.x,     leftTop.y,     leftTop.z    ); 
        glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
        glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
        glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
        glVertex3f(leftBottom.x,  leftBottom.y,  leftBottom.z ); 
        glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 
        glVertex3f(rightTop.x,    rightTop.y,    rightTop.z   ); 
        glVertex3f(rightBottom.x, rightBottom.y, rightBottom.z); 
        glVertex3f(rightCorner.x, rightCorner.y, rightCorner.z); 

        glEnd();
    }
  }
  

void Head::renderEyeBalls() {                                 
    
    // render white ball of left eyeball
    glPushMatrix();
        glColor3fv(EYEBALL_COLOR);
        glTranslatef(_leftEyePosition.x, _leftEyePosition.y, _leftEyePosition.z);        
        glutSolidSphere(EYEBALL_RADIUS, 30, 30);
    glPopMatrix();
    
    //render white ball of right eyeball
    glPushMatrix();
        glColor3fv(EYEBALL_COLOR);
        glTranslatef(_rightEyePosition.x, _rightEyePosition.y, _rightEyePosition.z);
        glutSolidSphere(EYEBALL_RADIUS, 30, 30);
    glPopMatrix();

    _irisProgram->bind();
    glBindTexture(GL_TEXTURE_2D, _irisTextureID);
    glEnable(GL_TEXTURE_2D);
    
    glm::quat orientation = getOrientation();
    glm::vec3 front = orientation * IDENTITY_FRONT;
    
    // render left iris
    glPushMatrix(); {
        glTranslatef(_leftEyePosition.x, _leftEyePosition.y, _leftEyePosition.z); //translate to eyeball position
        
        //rotate the eyeball to aim towards the lookat position
        glm::vec3 targetLookatVector = _lookAtPosition + _saccade - _leftEyePosition;
        glm::quat rotation = rotationBetween(front, targetLookatVector) * orientation;
        glm::vec3 rotationAxis = glm::axis(rotation);           
        glRotatef(glm::angle(rotation), rotationAxis.x, rotationAxis.y, rotationAxis.z);
        glTranslatef(0.0f, 0.0f, -IRIS_PROTRUSION);
        glScalef(IRIS_RADIUS * 2.0f, IRIS_RADIUS * 2.0f, IRIS_RADIUS); // flatten the iris
        
        // this ugliness is simply to invert the model transform and get the eye position in model space
        _irisProgram->setUniform(_eyePositionLocation, (glm::inverse(rotation) *
            (Application::getInstance()->getCamera()->getPosition() - _leftEyePosition) +
                glm::vec3(0.0f, 0.0f, IRIS_PROTRUSION)) * glm::vec3(1.0f / (IRIS_RADIUS * 2.0f),
                    1.0f / (IRIS_RADIUS * 2.0f), 1.0f / IRIS_RADIUS));
        
        glutSolidSphere(0.5f, 15, 15);
    }
    glPopMatrix();

    // render right iris
    glPushMatrix(); {
        glTranslatef(_rightEyePosition.x, _rightEyePosition.y, _rightEyePosition.z);  //translate to eyeball position       
        
        //rotate the eyeball to aim towards the lookat position
        glm::vec3 targetLookatVector = _lookAtPosition + _saccade - _rightEyePosition;
        glm::quat rotation = rotationBetween(front, targetLookatVector) * orientation;
        glm::vec3 rotationAxis = glm::axis(rotation);        
        glRotatef(glm::angle(rotation), rotationAxis.x, rotationAxis.y, rotationAxis.z);
        glTranslatef(0.0f, 0.0f, -IRIS_PROTRUSION);
        glScalef(IRIS_RADIUS * 2.0f, IRIS_RADIUS * 2.0f, IRIS_RADIUS); // flatten the iris
        
        // this ugliness is simply to invert the model transform and get the eye position in model space
        _irisProgram->setUniform(_eyePositionLocation, (glm::inverse(rotation) *
            (Application::getInstance()->getCamera()->getPosition() - _rightEyePosition) +
                glm::vec3(0.0f, 0.0f, IRIS_PROTRUSION)) * glm::vec3(1.0f / (IRIS_RADIUS * 2.0f),
                    1.0f / (IRIS_RADIUS * 2.0f), 1.0f / IRIS_RADIUS));
        
        glutSolidSphere(0.5f, 15, 15);
    }
    glPopMatrix();
    
    _irisProgram->release();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    
    glEnable(GL_RESCALE_NORMAL);
    glColor4f(_skinColor.x, _skinColor.y, _skinColor.z, _renderAlpha);
    
    // left eyelid
    glPushMatrix(); {
        glTranslatef(_leftEyePosition.x, _leftEyePosition.y, _leftEyePosition.z);  //translate to eyeball position
        glm::vec3 rotationAxis = glm::axis(orientation);
        glRotatef(glm::angle(orientation), rotationAxis.x, rotationAxis.y, rotationAxis.z);
        glScalef(EYELID_RADIUS, EYELID_RADIUS, EYELID_RADIUS);
        glRotatef(-40 - 50 * _leftEyeBlink, 1, 0, 0);
        Application::getInstance()->getGeometryCache()->renderHemisphere(15, 10);
        glRotatef(180 * _leftEyeBlink, 1, 0, 0);
        Application::getInstance()->getGeometryCache()->renderHemisphere(15, 10);
    }
    glPopMatrix();
    
    // right eyelid
    glPushMatrix(); {
        glTranslatef(_rightEyePosition.x, _rightEyePosition.y, _rightEyePosition.z);  //translate to eyeball position
        glm::vec3 rotationAxis = glm::axis(orientation);
        glRotatef(glm::angle(orientation), rotationAxis.x, rotationAxis.y, rotationAxis.z);
        glScalef(EYELID_RADIUS, EYELID_RADIUS, EYELID_RADIUS);
        glRotatef(-40 - 50 * _rightEyeBlink, 1, 0, 0);
        Application::getInstance()->getGeometryCache()->renderHemisphere(15, 10);
        glRotatef(180 * _rightEyeBlink, 1, 0, 0);
        Application::getInstance()->getGeometryCache()->renderHemisphere(15, 10);
    }
    glPopMatrix();
    
    glDisable(GL_RESCALE_NORMAL);
}

void Head::renderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition) {

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0);
    glBegin(GL_LINE_STRIP);
    glVertex3f(leftEyePosition.x, leftEyePosition.y, leftEyePosition.z);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glEnd();
    glBegin(GL_LINE_STRIP);
    glVertex3f(rightEyePosition.x, rightEyePosition.y, rightEyePosition.z);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glEnd();
}


void Head::updateHairPhysics(float deltaTime) {

    glm::quat orientation = getOrientation();
    glm::vec3 up    = orientation * IDENTITY_UP;
    glm::vec3 front = orientation * IDENTITY_FRONT;

    for (int t = 0; t < NUM_HAIR_TUFTS; t ++) {

        float fraction = (float)t / (float)(NUM_HAIR_TUFTS - 1);
        
        float angle = -20.0f + 40.0f * fraction;
        
        float radian = angle * PI_OVER_180;
        glm::vec3 baseDirection
        = front * sinf(radian)
        + up    * cosf(radian);
        
        _hairTuft[t].basePosition = _position + _scale * 0.9f * baseDirection;
                
        glm::vec3 midAxis = _hairTuft[t].midPosition - _hairTuft[t].basePosition;
        glm::vec3 endAxis = _hairTuft[t].endPosition - _hairTuft[t].midPosition;

        float midLength = glm::length(midAxis);        
        float endLength = glm::length(endAxis);

        glm::vec3 midDirection;
        glm::vec3 endDirection;
        
        if (midLength > 0.0f) {
            midDirection = midAxis / midLength;
        } else {
            midDirection = up;
        }
        
        if (endLength > 0.0f) {
            endDirection = endAxis / endLength;
        } else {
            endDirection = up;
        }
        
        // add spring force
        float midForce = midLength - _hairTuft[t].length * ONE_HALF;
        float endForce = endLength - _hairTuft[t].length * ONE_HALF;
        _hairTuft[t].midVelocity -= midDirection * midForce * HAIR_SPRING_FORCE * deltaTime;
        _hairTuft[t].endVelocity -= endDirection * endForce * HAIR_SPRING_FORCE * deltaTime;

        // add gravity force
        glm::vec3 gravityForce = _gravity * HAIR_GRAVITY_FORCE * deltaTime;
        _hairTuft[t].midVelocity += gravityForce;
        _hairTuft[t].endVelocity += gravityForce;
    
        // add torque force
        _hairTuft[t].midVelocity += baseDirection * HAIR_TORQUE_FORCE * deltaTime;
        _hairTuft[t].endVelocity += midDirection  * HAIR_TORQUE_FORCE * deltaTime;
        
        // add drag force
        float momentum = 1.0f - (HAIR_DRAG * deltaTime);
        if (momentum < 0.0f) {
            _hairTuft[t].midVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
            _hairTuft[t].endVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        } else {
            _hairTuft[t].midVelocity *= momentum;
            _hairTuft[t].endVelocity *= momentum;
        }
            
        // update position by velocity
        _hairTuft[t].midPosition  += _hairTuft[t].midVelocity;
        _hairTuft[t].endPosition  += _hairTuft[t].endVelocity;

        // clamp lengths
        glm::vec3 newMidVector = _hairTuft[t].midPosition - _hairTuft[t].basePosition;
        glm::vec3 newEndVector = _hairTuft[t].endPosition - _hairTuft[t].midPosition;

        float newMidLength = glm::length(newMidVector);
        float newEndLength = glm::length(newEndVector);
        
        glm::vec3 newMidDirection;
        glm::vec3 newEndDirection;

        if (newMidLength > 0.0f) {
            newMidDirection = newMidVector/newMidLength;
        } else {
            newMidDirection = up;
        }

        if (newEndLength > 0.0f) {
            newEndDirection = newEndVector/newEndLength;
        } else {
            newEndDirection = up;
        }
        
        _hairTuft[t].endPosition = _hairTuft[t].midPosition  + newEndDirection * _hairTuft[t].length * ONE_HALF;
        _hairTuft[t].midPosition = _hairTuft[t].basePosition + newMidDirection * _hairTuft[t].length * ONE_HALF;
    }
}

