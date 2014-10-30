//
//  RenderableLightEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <FBXReader.h>

#include "InterfaceConfig.h"

#include <PerfStat.h>
#include <LightEntityItem.h>


#include "Application.h"
#include "Menu.h"
#include "EntityTreeRenderer.h"
#include "RenderableLightEntityItem.h"


EntityItem* RenderableLightEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableLightEntityItem(entityID, properties);
}

void RenderableLightEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableLightEntityItem::render");
    assert(getType() == EntityTypes::Light);
    glm::vec3 position = getPositionInMeters();
    glm::vec3 center = getCenterInMeters();
    glm::vec3 dimensions = getDimensions() * (float)TREE_SCALE;
    glm::quat rotation = getRotation();
    float largestDiameter = glm::max(dimensions.x, dimensions.y, dimensions.z);

    const float MAX_COLOR = 255.0f;
    float diffuseR = getDiffuseColor()[RED_INDEX] / MAX_COLOR;
    float diffuseG = getDiffuseColor()[GREEN_INDEX] / MAX_COLOR;
    float diffuseB = getDiffuseColor()[BLUE_INDEX] / MAX_COLOR;

    float ambientR = getAmbientColor()[RED_INDEX] / MAX_COLOR;
    float ambientG = getAmbientColor()[GREEN_INDEX] / MAX_COLOR;
    float ambientB = getAmbientColor()[BLUE_INDEX] / MAX_COLOR;

    float specularR = getSpecularColor()[RED_INDEX] / MAX_COLOR;
    float specularG = getSpecularColor()[GREEN_INDEX] / MAX_COLOR;
    float specularB = getSpecularColor()[BLUE_INDEX] / MAX_COLOR;

    glm::vec3 ambient = glm::vec3(ambientR, ambientG, ambientB);
    glm::vec3 diffuse = glm::vec3(diffuseR, diffuseG, diffuseB);
    glm::vec3 specular = glm::vec3(specularR, specularG, specularB);
    glm::vec3 direction = IDENTITY_FRONT * rotation;
    float constantAttenuation = getConstantAttenuation();
    float linearAttenuation = getLinearAttenuation();
    float quadraticAttenuation = getQuadraticAttenuation();
    float exponent = getExponent();
    float cutoff = glm::radians(getCutoff());

    if (_isSpotlight) {
        Application::getInstance()->getDeferredLightingEffect()->addSpotLight(position, largestDiameter / 2.0f, 
            ambient, diffuse, specular, constantAttenuation, linearAttenuation, quadraticAttenuation,
            direction, exponent, cutoff);
    } else {
        Application::getInstance()->getDeferredLightingEffect()->addPointLight(position, largestDiameter / 2.0f, 
            ambient, diffuse, specular, constantAttenuation, linearAttenuation, quadraticAttenuation);
    }

    bool wantDebug = false;
    if (wantDebug) {
        glColor4f(diffuseR, diffuseG, diffuseB, 1.0f);
        glPushMatrix();
            glTranslatef(position.x, position.y, position.z);
            glm::vec3 axis = glm::axis(rotation);
            glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
            glPushMatrix();
                glm::vec3 positionToCenter = center - position;
                glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);

                glScalef(dimensions.x, dimensions.y, dimensions.z);
                Application::getInstance()->getDeferredLightingEffect()->renderWireSphere(0.5f, 15, 15);
            glPopMatrix();
        glPopMatrix();
    }
};