//
//  Skeleton.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "Skeleton.h"
#include "Util.h"
#include "world.h"

const float BODY_SPRING_DEFAULT_TIGHTNESS = 1000.0f;
const float FLOATING_HEIGHT               = 0.13f;

Skeleton::Skeleton() {
}

void Skeleton::initialize() {    
    
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        joint[b].parent              = AVATAR_JOINT_NULL;
        joint[b].position            = glm::vec3(0.0, 0.0, 0.0);
        joint[b].rotation            = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        joint[b].length              = 0.0;
        joint[b].bindRadius          = 1.0f / 8;
    }
    
    // put the arms at the side
    joint[AVATAR_JOINT_LEFT_ELBOW].rotation = glm::quat(glm::vec3(0.0f, 0.0f, PIf * 0.5f));
    joint[AVATAR_JOINT_RIGHT_ELBOW].rotation = glm::quat(glm::vec3(0.0f, 0.0f, -PIf * 0.5f));
    
    // bend the knees
    joint[AVATAR_JOINT_LEFT_KNEE].rotation = joint[AVATAR_JOINT_RIGHT_KNEE].rotation =
        glm::quat(glm::vec3(PIf / 8.0f, 0.0f, 0.0f));
    joint[AVATAR_JOINT_LEFT_HEEL].rotation = joint[AVATAR_JOINT_RIGHT_HEEL].rotation =
        glm::quat(glm::vec3(-PIf / 4.0f, 0.0f, 0.0f));
    
    // specify the parental hierarchy
    joint[ AVATAR_JOINT_PELVIS		      ].parent = AVATAR_JOINT_NULL;
    joint[ AVATAR_JOINT_TORSO             ].parent = AVATAR_JOINT_PELVIS;
    joint[ AVATAR_JOINT_CHEST		      ].parent = AVATAR_JOINT_TORSO;
    joint[ AVATAR_JOINT_NECK_BASE	      ].parent = AVATAR_JOINT_CHEST;
    joint[ AVATAR_JOINT_HEAD_BASE         ].parent = AVATAR_JOINT_NECK_BASE;
    joint[ AVATAR_JOINT_HEAD_TOP          ].parent = AVATAR_JOINT_HEAD_BASE;
    joint[ AVATAR_JOINT_LEFT_COLLAR       ].parent = AVATAR_JOINT_CHEST;
    joint[ AVATAR_JOINT_LEFT_SHOULDER     ].parent = AVATAR_JOINT_LEFT_COLLAR;
    joint[ AVATAR_JOINT_LEFT_ELBOW	      ].parent = AVATAR_JOINT_LEFT_SHOULDER;
    joint[ AVATAR_JOINT_LEFT_WRIST		  ].parent = AVATAR_JOINT_LEFT_ELBOW;
    joint[ AVATAR_JOINT_LEFT_FINGERTIPS   ].parent = AVATAR_JOINT_LEFT_WRIST;
    joint[ AVATAR_JOINT_RIGHT_COLLAR      ].parent = AVATAR_JOINT_CHEST;
    joint[ AVATAR_JOINT_RIGHT_SHOULDER	  ].parent = AVATAR_JOINT_RIGHT_COLLAR;
    joint[ AVATAR_JOINT_RIGHT_ELBOW	      ].parent = AVATAR_JOINT_RIGHT_SHOULDER;
    joint[ AVATAR_JOINT_RIGHT_WRIST       ].parent = AVATAR_JOINT_RIGHT_ELBOW;
    joint[ AVATAR_JOINT_RIGHT_FINGERTIPS  ].parent = AVATAR_JOINT_RIGHT_WRIST;
    joint[ AVATAR_JOINT_LEFT_HIP		  ].parent = AVATAR_JOINT_PELVIS;
    joint[ AVATAR_JOINT_LEFT_KNEE		  ].parent = AVATAR_JOINT_LEFT_HIP;
    joint[ AVATAR_JOINT_LEFT_HEEL		  ].parent = AVATAR_JOINT_LEFT_KNEE;
    joint[ AVATAR_JOINT_LEFT_TOES		  ].parent = AVATAR_JOINT_LEFT_HEEL;
    joint[ AVATAR_JOINT_RIGHT_HIP		  ].parent = AVATAR_JOINT_PELVIS;
    joint[ AVATAR_JOINT_RIGHT_KNEE		  ].parent = AVATAR_JOINT_RIGHT_HIP;
    joint[ AVATAR_JOINT_RIGHT_HEEL		  ].parent = AVATAR_JOINT_RIGHT_KNEE;
    joint[ AVATAR_JOINT_RIGHT_TOES		  ].parent = AVATAR_JOINT_RIGHT_HEEL;
    
    // specify the bind pose position
    joint[ AVATAR_JOINT_PELVIS           ].bindPosePosition = glm::vec3(  0.0,   0.0,    0.0  );
    joint[ AVATAR_JOINT_TORSO            ].bindPosePosition = glm::vec3(  0.0,   0.09,  -0.01 );
    joint[ AVATAR_JOINT_CHEST            ].bindPosePosition = glm::vec3(  0.0,   0.09,  -0.01 );
    joint[ AVATAR_JOINT_NECK_BASE        ].bindPosePosition = glm::vec3(  0.0,   0.14,   0.01 );
    joint[ AVATAR_JOINT_HEAD_BASE        ].bindPosePosition = glm::vec3(  0.0,   0.04,   0.00 );
    joint[ AVATAR_JOINT_HEAD_TOP         ].bindPosePosition = glm::vec3(  0.0,   0.04,   0.00 );
    
    joint[ AVATAR_JOINT_LEFT_COLLAR      ].bindPosePosition = glm::vec3( -0.06,  0.04,   0.01 );
    joint[ AVATAR_JOINT_LEFT_SHOULDER    ].bindPosePosition = glm::vec3( -0.05,  0.0,    0.01 );
    joint[ AVATAR_JOINT_LEFT_ELBOW       ].bindPosePosition = glm::vec3( -0.16,  0.0,    0.0  );
    joint[ AVATAR_JOINT_LEFT_WRIST       ].bindPosePosition = glm::vec3( -0.12,  0.0,    0.0  );
    joint[ AVATAR_JOINT_LEFT_FINGERTIPS  ].bindPosePosition = glm::vec3( -0.1,   0.0,    0.0  );
    
    joint[ AVATAR_JOINT_RIGHT_COLLAR     ].bindPosePosition = glm::vec3(  0.06,  0.04,   0.01 );
    joint[ AVATAR_JOINT_RIGHT_SHOULDER   ].bindPosePosition = glm::vec3(  0.05,  0.0,    0.01 );
    joint[ AVATAR_JOINT_RIGHT_ELBOW      ].bindPosePosition = glm::vec3(  0.16,  0.0,    0.0  );
    joint[ AVATAR_JOINT_RIGHT_WRIST      ].bindPosePosition = glm::vec3(  0.12,  0.0,    0.0  );
    joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].bindPosePosition = glm::vec3(  0.1,   0.0,    0.0  );
    
    joint[ AVATAR_JOINT_LEFT_HIP         ].bindPosePosition = glm::vec3( -0.05,  0.0,    0.02 );
    joint[ AVATAR_JOINT_LEFT_KNEE        ].bindPosePosition = glm::vec3(  0.00, -0.25,   0.00 );
    joint[ AVATAR_JOINT_LEFT_HEEL        ].bindPosePosition = glm::vec3(  0.00, -0.23,   0.00 );
    joint[ AVATAR_JOINT_LEFT_TOES        ].bindPosePosition = glm::vec3(  0.00,  0.00,  -0.06 );
    
    joint[ AVATAR_JOINT_RIGHT_HIP        ].bindPosePosition = glm::vec3(  0.05,  0.0,    0.02 );
    joint[ AVATAR_JOINT_RIGHT_KNEE       ].bindPosePosition = glm::vec3(  0.00, -0.25,   0.00 );
    joint[ AVATAR_JOINT_RIGHT_HEEL       ].bindPosePosition = glm::vec3(  0.00, -0.23,   0.00 );
    joint[ AVATAR_JOINT_RIGHT_TOES       ].bindPosePosition = glm::vec3(  0.00,  0.00,  -0.06 );
    
    // calculate bone length, absolute bind positions/rotations
    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        joint[b].length = glm::length(joint[b].bindPosePosition);
        
        if (joint[b].parent == AVATAR_JOINT_NULL) {
            joint[b].absoluteBindPosePosition = joint[b].bindPosePosition;
            joint[b].absoluteBindPoseRotation = glm::quat();
        } else {
            joint[b].absoluteBindPosePosition = joint[ joint[b].parent ].absoluteBindPosePosition +
                joint[b].bindPosePosition;
            glm::vec3 parentDirection = joint[ joint[b].parent ].absoluteBindPoseRotation * JOINT_DIRECTION;
            joint[b].absoluteBindPoseRotation = rotationBetween(parentDirection, joint[b].bindPosePosition) *
                joint[ joint[b].parent ].absoluteBindPoseRotation; 
        }
    }
}

// calculate positions and rotations of all bones by traversing the skeleton tree:
void Skeleton::update(float deltaTime, const glm::quat& orientation, glm::vec3 position) {

    for (int b = 0; b < NUM_AVATAR_JOINTS; b++) {
        if (joint[b].parent == AVATAR_JOINT_NULL) {
            joint[b].absoluteRotation = orientation * joint[b].rotation;
            joint[b].position = position;
        }
        else {
            joint[b].absoluteRotation = joint[ joint[b].parent ].absoluteRotation * joint[b].rotation;
            joint[b].position = joint[ joint[b].parent ].position;
        }

        glm::vec3 rotatedJointVector = joint[b].absoluteRotation * joint[b].bindPosePosition;
        joint[b].position += rotatedJointVector;
    }    
}


float Skeleton::getArmLength() {
    return joint[ AVATAR_JOINT_RIGHT_ELBOW      ].length
         + joint[ AVATAR_JOINT_RIGHT_WRIST	    ].length
         + joint[ AVATAR_JOINT_RIGHT_FINGERTIPS ].length;
}

float Skeleton::getHeight() {
    return 
        joint[ AVATAR_JOINT_LEFT_HEEL ].length +
        joint[ AVATAR_JOINT_LEFT_KNEE ].length +
        joint[ AVATAR_JOINT_PELVIS    ].length +
        joint[ AVATAR_JOINT_TORSO     ].length +
        joint[ AVATAR_JOINT_CHEST     ].length +
        joint[ AVATAR_JOINT_NECK_BASE ].length +
        joint[ AVATAR_JOINT_HEAD_BASE ].length;
}

float Skeleton::getPelvisStandingHeight() {
    return joint[ AVATAR_JOINT_LEFT_HEEL ].length +
           joint[ AVATAR_JOINT_LEFT_KNEE ].length;
}

float Skeleton::getPelvisFloatingHeight() {
    return joint[ AVATAR_JOINT_LEFT_HEEL ].length +
           joint[ AVATAR_JOINT_LEFT_KNEE ].length +
           FLOATING_HEIGHT;
}

float Skeleton::getPelvisToHeadLength() {
    return 
        joint[ AVATAR_JOINT_TORSO     ].length +
        joint[ AVATAR_JOINT_CHEST     ].length +
        joint[ AVATAR_JOINT_NECK_BASE ].length +
        joint[ AVATAR_JOINT_HEAD_BASE ].length; 
}



