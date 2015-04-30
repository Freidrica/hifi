//
//  CameraToolBox.h
//  interface/src/devices
//
//  Created by David Rowe on 30 Apr 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CameraToolBox_h
#define hifi_CameraToolBox_h

#include <DependencyManager.h>
#include <GeometryCache.h>

class CameraToolBox : public Dependency {
    SINGLETON_DEPENDENCY

public:
    void render(int x, int y, bool boxed);
    bool mousePressEvent(int x, int y);

protected:
    CameraToolBox();

private:
    GLuint _enabledTextureId = 0;
    GLuint _mutedTextureId = 0;
    int _boxQuadID = GeometryCache::UNKNOWN_ID;
    QRect _iconBounds;
    qint64 _iconPulseTimeReference = 0;
};

#endif // hifi_CameraToolBox_h