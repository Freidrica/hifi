//
//  movable.js
//  examples/entityScripts
//
//  Created by Brad Hefta-Gaub on 11/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
(function(){ 

    this.entityID = null;
    this.properties = null;
    this.graboffset = null;
    this.clickedAt = null;
    this.firstHolding = true;
    this.clickedX = -1;
    this.clickedY = -1;
    this.rotateOverlayTarget = null;
    this.rotateOverlayInner = null;
    this.rotateOverlayOuter = null;
    this.rotateOverlayCurrent = null;
    this.rotateMode = false;
    this.originalRotation = null;
    this.sound = null;
    this.injector = null;
    
    var rotateOverlayTargetSize = 10000; // really big target
    var innerSnapAngle = 22.5; // the angle which we snap to on the inner rotation tool
    var innerRadius;
    var outerRadius;
    var yawCenter;
    var yawZero;
    var rotationNormal;
    var yawNormal;
    
    var debug = true;
    
    // Download sound if needed
    this.maybeDownloadSound = function() {
        if (this.sound === null) {
            this.sound = SoundCache.getSound("http://public.highfidelity.io/sounds/Collisions-otherorganic/whoosh2.raw");
        }
    }
    // Play drag sound
    this.playSound = function() {
        this.stopSound();
        if (this.sound && this.sound.downloaded) {
            this.injector = Audio.playSound(this.sound, { position: this.properties.position, loop: true, volume: 0.1 });
        }
    }

    // stop drag sound
    this.stopSound = function() {
        if (this.injector) {
            Audio.stopInjector(this.injector);
            this.injector = null;
        }
    }

    // Pr, Vr are respectively the Ray's Point of origin and Vector director
    // Pp, Np are respectively the Plane's Point of origin and Normal vector
    this.rayPlaneIntersection = function(Pr, Vr, Pp, Np) {
        var d = -Vec3.dot(Pp, Np);
        var t = -(Vec3.dot(Pr, Np) + d) / Vec3.dot(Vr, Np);
        return Vec3.sum(Pr, Vec3.multiply(t, Vr));
    };

    // updates the piece position based on mouse input
    this.updatePosition = function(mouseEvent) {
        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var upVector = { x: 0, y: 1, z: 0 };
        var intersection = this.rayPlaneIntersection(pickRay.origin, pickRay.direction,
                                                     this.properties.position, upVector);
                                                     
        var newPosition = Vec3.sum(intersection, this.graboffset);
        Entities.editEntity(this.entityID, { position: newPosition });
    };

    this.grab = function(mouseEvent) {
        // first calculate the offset
        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var upVector = { x: 0, y: 1, z: 0 };
        var intersection = this.rayPlaneIntersection(pickRay.origin, pickRay.direction,
                                                     this.properties.position, upVector);                                 
        this.graboffset = Vec3.subtract(this.properties.position, intersection);
    };
    
    this.move = function(mouseEvent) {
        this.updatePosition(mouseEvent);
        if (this.injector === null) {
            this.playSound();
        }
    };
    
    this.release = function(mouseEvent) {
        this.updatePosition(mouseEvent);
    };
    
    this.rotate = function(mouseEvent) {
        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var result = Overlays.findRayIntersection(pickRay);

        if (result.intersects) {
            var center = yawCenter;
            var zero = yawZero;
            var centerToZero = Vec3.subtract(center, zero);
            var centerToIntersect = Vec3.subtract(center, result.intersection);
            var angleFromZero = Vec3.orientedAngle(centerToZero, centerToIntersect, rotationNormal);
            
            var distanceFromCenter = Vec3.distance(center, result.intersection);
            var snapToInner = false;
            // var innerRadius = (Vec3.length(selectionManager.worldDimensions) / 2) * 1.1;
            if (distanceFromCenter < innerRadius) {
                angleFromZero = Math.floor(angleFromZero/innerSnapAngle) * innerSnapAngle;
                snapToInner = true;
            }
            
            var yawChange = Quat.fromVec3Degrees({ x: 0, y: angleFromZero, z: 0 });
            Entities.editEntity(this.entityID, { rotation: Quat.multiply(yawChange, this.originalRotation) });
            

            // update the rotation display accordingly...
            var startAtCurrent = 360-angleFromZero;
            var endAtCurrent = 360;
            var startAtRemainder = 0;
            var endAtRemainder = 360-angleFromZero;
            if (angleFromZero < 0) {
                startAtCurrent = 0;
                endAtCurrent = -angleFromZero;
                startAtRemainder = -angleFromZero;
                endAtRemainder = 360;
            }

            if (snapToInner) {
                Overlays.editOverlay(this.rotateOverlayOuter, { startAt: 0, endAt: 360 });
                Overlays.editOverlay(this.rotateOverlayInner, { startAt: startAtRemainder, endAt: endAtRemainder });
                Overlays.editOverlay(this.rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: innerRadius,
                                                                majorTickMarksAngle: innerSnapAngle, minorTickMarksAngle: 0,
                                                                majorTickMarksLength: -0.25, minorTickMarksLength: 0, });
            } else {
                Overlays.editOverlay(this.rotateOverlayInner, { startAt: 0, endAt: 360 });
                Overlays.editOverlay(this.rotateOverlayOuter, { startAt: startAtRemainder, endAt: endAtRemainder });
                Overlays.editOverlay(this.rotateOverlayCurrent, { startAt: startAtCurrent, endAt: endAtCurrent, size: outerRadius,
                                                                majorTickMarksAngle: 45.0, minorTickMarksAngle: 5,
                                                                majorTickMarksLength: 0.25, minorTickMarksLength: 0.1, });
            }
        }    
    };
      // All callbacks start by updating the properties
    this.updateProperties = function(entityID) {
        if (this.entityID === null || !this.entityID.isKnownID) {
            this.entityID = Entities.identifyEntity(entityID);
        }
        this.properties = Entities.getEntityProperties(this.entityID);
    };

    this.cleanupRotateOverlay = function() {
        Overlays.deleteOverlay(this.rotateOverlayTarget);
        Overlays.deleteOverlay(this.rotateOverlayInner);
        Overlays.deleteOverlay(this.rotateOverlayOuter);
        Overlays.deleteOverlay(this.rotateOverlayCurrent);
        this.rotateOverlayTarget = null;
        this.rotateOverlayInner = null;
        this.rotateOverlayOuter = null;
        this.rotateOverlayCurrent = null;
    }
    
    this.displayRotateOverlay = function(mouseEvent) {
        var yawOverlayAngles = { x: 90, y: 0, z: 0 };
        var yawOverlayRotation = Quat.fromVec3Degrees(yawOverlayAngles);

        yawNormal   = { x: 0, y: 1, z: 0 };
        yawCenter = this.properties.position;
        rotationNormal = yawNormal;

        // Size the overlays to the current selection size
        var diagonal = (Vec3.length(this.properties.dimensions) / 2) * 1.1;
        var halfDimensions = Vec3.multiply(this.properties.dimensions, 0.5);
        innerRadius = diagonal;
        outerRadius = diagonal * 1.15;
        var innerAlpha = 0.2;
        var outerAlpha = 0.2;

        this.rotateOverlayTarget = Overlays.addOverlay("circle3d", {
                        position: this.properties.position,
                        size: 10000,
                        color: { red: 0, green: 0, blue: 0 },
                        alpha: 0.0,
                        solid: true,
                        visible: true,
                        rotation: yawOverlayRotation,
                        ignoreRayIntersection: false
                    });

        this.rotateOverlayInner = Overlays.addOverlay("circle3d", {
                    position: this.properties.position,
                    size: innerRadius,
                    innerRadius: 0.9,
                    alpha: innerAlpha,
                    color: { red: 51, green: 152, blue: 203 },
                    solid: true,
                    visible: true,
                    rotation: yawOverlayRotation,
                    hasTickMarks: true,
                    majorTickMarksAngle: innerSnapAngle,
                    minorTickMarksAngle: 0,
                    majorTickMarksLength: -0.25,
                    minorTickMarksLength: 0,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                });

        this.rotateOverlayOuter = Overlays.addOverlay("circle3d", {
                    position: this.properties.position,
                    size: outerRadius,
                    innerRadius: 0.9,
                    startAt: 0,
                    endAt: 360,
                    alpha: outerAlpha,
                    color: { red: 51, green: 152, blue: 203 },
                    solid: true,
                    visible: true,
                    rotation: yawOverlayRotation,

                    hasTickMarks: true,
                    majorTickMarksAngle: 45.0,
                    minorTickMarksAngle: 5,
                    majorTickMarksLength: 0.25,
                    minorTickMarksLength: 0.1,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    ignoreRayIntersection: true, // always ignore this
                });

        this.rotateOverlayCurrent = Overlays.addOverlay("circle3d", {
                    position: this.properties.position,
                    size: outerRadius,
                    startAt: 0,
                    endAt: 0,
                    innerRadius: 0.9,
                    color: { red: 224, green: 67, blue: 36},
                    alpha: 0.8,
                    solid: true,
                    visible: true,
                    rotation: yawOverlayRotation,
                    ignoreRayIntersection: true, // always ignore this
                    hasTickMarks: true,
                    majorTickMarksColor: { red: 0, green: 0, blue: 0 },
                    minorTickMarksColor: { red: 0, green: 0, blue: 0 },
                });

        var pickRay = Camera.computePickRay(mouseEvent.x, mouseEvent.y)
        var result = Overlays.findRayIntersection(pickRay);
        yawZero = result.intersection;
                
    };
    
    this.preload = function(entityID) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.maybeDownloadSound();
    };
    
    this.clickDownOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        this.grab(mouseEvent);

        var d = new Date();
        this.clickedAt = d.getTime();
        this.firstHolding = true;
        
        this.clickedX = mouseEvent.x;
        this.clickedY = mouseEvent.y;
    };

    this.holdingClickOnEntity = function(entityID, mouseEvent) {

        this.updateProperties(entityID); // All callbacks start by updating the properties

        if (this.firstHolding) {
            // if we haven't moved yet...
            if (this.clickedX == mouseEvent.x && this.clickedY == mouseEvent.y) {
                var d = new Date();
                var now = d.getTime();
        
                if (now - this.clickedAt > 500) {
                    this.displayRotateOverlay(mouseEvent);
                    this.firstHolding = false;
                    this.rotateMode = true;
                    this.originalRotation = this.properties.rotation;
                }
            } else {
                this.firstHolding = false;
            }
        }

        if (this.rotateMode) {
            this.rotate(mouseEvent);
        } else {
            this.move(mouseEvent);
        }
    };
    this.clickReleaseOnEntity = function(entityID, mouseEvent) {
        this.updateProperties(entityID); // All callbacks start by updating the properties
        if (this.rotateMode) {
            this.rotate(mouseEvent);
        } else {
            this.release(mouseEvent);
        }
        
        if (this.rotateOverlayTarget != null) {
            this.cleanupRotateOverlay();
            this.rotateMode = false;
        }

        this.firstHolding = false;
        this.stopSound();
    };

})