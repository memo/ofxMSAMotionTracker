/***********************************************************************
 -----------------------------------
 
 Copyright (c) 2008, Memo Akten, www.memo.tv
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 ***********************************************************************/


#include "MSACore.h"
#include "MSAMotionTracker.h"
#include "ofxSimpleGuiToo.h"
//#include "MSAUtils.h"

namespace msa {

MotionTracker    tracker;

MotionTracker::MotionTracker() {
//    captureImage        = NULL;
//    resizedImage        = NULL;
//    maskedImage            = NULL;
}



//MotionTracker::~MotionTracker() {
////    clear();
////    if(captureImage) delete captureImage;
////    if(maskedImage) delete maskedImage;
//    //    if(currentScaledImage) delete currentScaledImage;
//}


void MotionTracker::clear() {
    captureImage = nullptr;
    resizedImage = nullptr;
    resizedImage = nullptr;
//    DelPointer(captureImage);
//    DelPointer(resizedImage);
//    DelPointer(maskedImage);
}



void MotionTracker::setup() {
    ofLog(OF_LOG_VERBOSE, "MotionTracker::setup");

    //        char* libraries;
    //        char* modules;
    //        cvGetModuleInfo( 0, (const char**)&libraries, (const char**)&modules );
    //        printf("******************************\n");
    //        printf("Libraries: %s\nModules: %s\n", libraries, modules );
    //        printf("******************************\n");

    setupUI();

    //        setDataPathToBundle();
    imgTrackerLearning.load("settings/Tracker/tracker-learning.png");
    imgCameraIcon.load("settings/Tracker/camera-icon.png");
    doneSound.load("settings/Tracker/Glass.aiff", false);
    learningSound.load("settings/Tracker/Blow.aiff", false);
    learningSound.setLoop(true);
    //        restoreDataPath();

    //        allocateTextures(inputImage);

    learnStartTime            = ofGetElapsedTimef();
    isLearning                = false;

    ofxSimpleGuiPage &trackerPage = gui.page("TRACKER CONFIG");
    trackerPage.addContent("cleanPlate", cleanPlate);
    trackerPage.addContent("currentGreyImage", currentGreyImage);
    trackerPage.addContent("processedGreyImage", processedGreyImage);
    //    trackerPage.addContent("maskedImage", maskedImage);
    trackerPage.addContent("currentDiff", currentDiff);
    trackerPage.addContent("accDiff", accDiff);
    trackerPage.addContent("opticalFlow", opticalFlow);
    trackerPage.addContent("Contours", contourFinder);

    ofxSimpleGuiPage &segmentPage = gui.page("SEGMENTATION");
    segmentPage.addContent("processedGreyImage", processedGreyImage);
    segmentPage.addContent("currentDiff", currentDiff);
    trackerPage.addContent("accDiff", accDiff);
    segmentPage.addContent("Contours", contourFinder);


    //        gui.page("WARP").addQuadWarper("inputPoints", *captureImage, settings.inputPoints).newColumn = true;
    //        gui.loadFromXML();

    reset();

    ofAddListener(ofEvents().keyPressed, this, &MotionTracker::keyPressed);
}



void MotionTracker::allocateTextures(ofPixelsRef inputImage) {
    ofLogNotice() << "MotionTracker::allocateTextures()";

//    DelPointer(captureImage);
//    DelPointer(resizedImage);
//    DelPointer(maskedImage);

    if(inputImage.getNumChannels() > 1) {
        captureImage = make_shared<ofxCvColorImage>();
        if(settings.scaleDown>1) resizedImage = make_shared<ofxCvColorImage>();
        else resizedImage = captureImage;
        maskedImage = make_shared<ofxCvGrayscaleImage>();
    } else {
        captureImage = make_shared<ofxCvGrayscaleImage>();
        if(settings.scaleDown>1) resizedImage = make_shared<ofxCvGrayscaleImage>();
        else resizedImage = captureImage;
        maskedImage = make_shared<ofxCvGrayscaleImage>();
    }

    opticalFlowMult = 1.0f/opticalFlow.scaleDown;

    if(settings.scaleDown==0) settings.scaleDown = 1;
    size.x = inputImage.getWidth()/settings.scaleDown;
    size.y = inputImage.getHeight()/settings.scaleDown;

    invSize.x = 1.0f/size.x;
    invSize.y = 1.0f/size.y;

    Vec2f opticalFlowSize = size * opticalFlowMult;

    printf("\n real capture: %i x %i \n resized : %.0f x %.0f \n optical flow: %.0f x %.0f \n captureIsColor: %i \n\n", inputImage.getWidth(), inputImage.getHeight(), size.x, size.y, opticalFlowSize.x, opticalFlowSize.y, inputImage.getNumChannels() > 1);

    captureImage->allocate(inputImage.getWidth(), inputImage.getHeight());

    resizedImage->allocate(size.x, size.y);
    maskedImage->allocate(size.x, size.y);
    cleanPlate.allocate(size.x, size.y);
    processedCleanPlate.allocate(size.x, size.y);
    currentGreyImage.allocate(size.x, size.y);
    processedGreyImage.allocate(size.x, size.y);
    previousGreyImage.allocate(size.x, size.y);
    currentDiff.allocate(size.x, size.y);
    accDiff.allocate(size.x, size.y);
    opticalFlow.allocate(opticalFlowSize.x, opticalFlowSize.y);
    opFlowInput1.allocate(opticalFlow.getWidth(), opticalFlow.getHeight());
    opFlowInput2.allocate(opticalFlow.getWidth(), opticalFlow.getHeight());

    captureImage->setUseTexture(true);
    resizedImage->setUseTexture(true);
    maskedImage->setUseTexture(true);
    cleanPlate.setUseTexture(true);
    processedCleanPlate.setUseTexture(false);
    currentGreyImage.setUseTexture(true);
    processedGreyImage.setUseTexture(true);
    previousGreyImage.setUseTexture(false);
    currentDiff.setUseTexture(true);
    accDiff.setUseTexture(true);
    opFlowInput1.setUseTexture(false);
    opFlowInput2.setUseTexture(false);

    settings.inputPoints[0].set(0, 0);
    settings.inputPoints[1].set(size.x, 0);
    settings.inputPoints[2].set(size.x, size.y);
    settings.inputPoints[3].set(0, size.y);

    reset();
}

template<typename T>
void clearImage(T& img) {
    if(img.bAllocated) img.set(0);
}

void MotionTracker::reset() {
    memset(&stats, 0, sizeof(stats));

    if(captureImage) clearImage(*captureImage);
    if(resizedImage) clearImage(*resizedImage);
    if(maskedImage) clearImage(*maskedImage);
    clearImage(cleanPlate);
    clearImage(cleanPlate);
    clearImage(processedCleanPlate);
    clearImage(currentGreyImage);
    clearImage(processedGreyImage);
    clearImage(previousGreyImage);
    clearImage(currentDiff);
    clearImage(accDiff);
    clearImage(opFlowInput1);
    clearImage(opFlowInput2);
    frameCounter = 0;

    opticalFlow.reset();

    static ofImage imgCameraGrab;
    if(imgCameraGrab.load("settings/Tracker/cleanplate.png") && imgCameraGrab.getWidth() == cleanPlate.getWidth() && imgCameraGrab.getHeight() == cleanPlate.getHeight()) {
        cleanPlate = imgCameraGrab.getPixels();
        //cleanPlate.setFromPixels(imgCameraGrab.getPixels(), imgCameraGrab.getWidth(), imgCameraGrab.getHeight());
    }
}



//--------------------------------------------------------------
void MotionTracker::update(ofPixelsRef inputImage){
    if(!settings.enabled) return;

    if(inputImage.getWidth() == 0 || inputImage.getHeight() == 0) return;

    //        if(!isReady()) return;

    if(settings.reinit || captureImage == NULL || captureImage->getWidth() != inputImage.getWidth() || captureImage->getHeight() != inputImage.getHeight()) {
        settings.reinit = false;
        printf("MotionTracker: image size changed\n");
        allocateTextures(inputImage);
    }


    //        if(stats.hasNewFrame) {
    float nowTime = ofGetElapsedTimef();
    stats.captureFPS = 1.0f/(nowTime - stats.lastTimeHadFrame);
    stats.lastTimeHadFrame = nowTime;

    bool useBG = settings.subtractBG || settings.lessThanBG || settings.greaterThanBG;

    if(roiSettings.setRoi) {
        roiSettings.setRoi = false;
        reset();
        if(roiSettings.useRoi) {
            int x = roiSettings.roi.x1;
            int y = roiSettings.roi.y1;
            int w = roiSettings.roi.width();
            int h = roiSettings.roi.height();

            int xo = x * opticalFlowMult;
            int yo = y * opticalFlowMult;
            int wo = w * opticalFlowMult;
            int ho = h * opticalFlowMult;

            captureImage->setROI(x, y, w, h);
            resizedImage->setROI(x, y, w, h);
            maskedImage->setROI(x, y, w, h);
            //                currentScaledImage->setROI(x, y, w, h);
            cleanPlate.setROI(x, y, w, h);
            processedCleanPlate.setROI(x, y, w, h);
            currentGreyImage.setROI(x, y, w, h);
            processedGreyImage.setROI(x, y, w, h);
            previousGreyImage.setROI(x, y, w, h);
            currentDiff.setROI(x, y, w, h);
            accDiff.setROI(x, y, w, h);
            opFlowInput1.setROI(xo, yo, wo, ho);
            opFlowInput2.setROI(xo, yo, wo, ho);
            opticalFlow.setROI(xo, yo, wo, ho);
        } else {
            if(getWidth() > 0 && getHeight() > 0) {
                captureImage->resetROI();
                resizedImage->resetROI();
                maskedImage->resetROI();
                //                currentScaledImage->resetROI();
                cleanPlate.resetROI();
                processedCleanPlate.resetROI();
                currentGreyImage.resetROI();
                processedGreyImage.resetROI();
                previousGreyImage.resetROI();
                currentDiff.resetROI();
                accDiff.resetROI();
                opFlowInput1.resetROI();
                opFlowInput2.resetROI();
                opticalFlow.resetROI();
            }
        }
    }


    captureImage->setFromPixels(inputImage);
    if(settings.scaleDown) {
        resizedImage->scaleIntoMe(*captureImage);
    }

    if(settings.transform.flipVideoX || settings.transform.flipVideoY) resizedImage->mirror(settings.transform.flipVideoY, settings.transform.flipVideoX);                    // mirror it

    previousGreyImage = currentGreyImage;                            // save frame for next frame
    opFlowInput2 = opFlowInput1;

    if(inputImage.getNumChannels() > 1) {
        currentGreyImage = *dynamic_pointer_cast<ofxCvColorImage>(resizedImage);
    } else {
        currentGreyImage = *dynamic_pointer_cast<ofxCvGrayscaleImage>(resizedImage);
    }

    if(settings.doWarpInput) {
        currentGreyImage.warpPerspective(settings.inputPoints[0], settings.inputPoints[1], settings.inputPoints[2], settings.inputPoints[3]);
    }

    if(useBG) processedCleanPlate = cleanPlate;

//    if(settings.transform.flipVideoX || settings.transform.flipVideoY) currentGreyImage.mirror(settings.transform.flipVideoY, settings.transform.flipVideoX);                    // mirror it
    if(settings.pre.blur)     {
        cvSmooth(currentGreyImage.getCvImage(), currentGreyImage.getCvImage(), CV_BLUR , settings.pre.blur*2+1);
        if(useBG) cvSmooth(processedCleanPlate.getCvImage(), processedCleanPlate.getCvImage(), CV_BLUR , settings.pre.blur*2+1);
    }

    processedGreyImage = currentGreyImage;
    //        if(settings.bottomThreshold) cvThreshold(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.bottomThreshold, 255, CV_THRESH_TOZERO);




    opFlowInput1.scaleIntoMe(currentGreyImage);


    if(frameCounter > 2) {        // dont do anything until we have enough in history
        // do current difference
        if(settings.diff.enabled) {
            currentDiff.absDiff(currentGreyImage, previousGreyImage);
            if(settings.diff.threshold) cvThreshold(currentDiff.getCvImage(), currentDiff.getCvImage(), settings.diff.threshold, 255, CV_THRESH_BINARY);
            if(settings.diff.blur1) cvSmooth(currentDiff.getCvImage(), currentDiff.getCvImage(), CV_BLUR , settings.diff.blur1*2+1);
            if(settings.diff.openAmount) cvMorphologyEx(currentDiff.getCvImage(), currentDiff.getCvImage(), NULL, NULL, CV_MOP_OPEN, settings.diff.openAmount);
            if(settings.diff.closeAmount) cvMorphologyEx(currentDiff.getCvImage(), currentDiff.getCvImage(), NULL, NULL, CV_MOP_CLOSE, settings.diff.closeAmount);
            if(settings.diff.blur2) cvSmooth(currentDiff.getCvImage(), currentDiff.getCvImage(), CV_BLUR , settings.diff.blur2*2+1);

            if(settings.accDiff.enabled) {
                if(settings.accDiff.blur1) cvSmooth(accDiff.getCvImage(), accDiff.getCvImage(), CV_BLUR , settings.accDiff.blur1*2+1);
                cvAddWeighted(accDiff.getCvImage(), settings.accDiff.oldWeight, currentDiff.getCvImage(), settings.accDiff.newWeight, 0, accDiff.getCvImage());
                if(settings.accDiff.blur2) cvSmooth(accDiff.getCvImage(), accDiff.getCvImage(), CV_BLUR , settings.accDiff.blur2*2+1);
                if(settings.accDiff.bottomThreshold) cvThreshold(accDiff.getCvImage(), accDiff.getCvImage(), settings.accDiff.bottomThreshold, 255, CV_THRESH_TOZERO);
                if(settings.accDiff.adaptiveThreshold) {
                    cvAdaptiveThreshold(accDiff.getCvImage(), accDiff.getCvImage(), 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 2*settings.accDiff.adaptiveBlockSize+1, settings.accDiff.adaptiveThreshold);
                    accDiff.invert();
                }
                accDiff.flagImageChanged();

            }
        }

        float f = 1.0f/(settings.averageFrames+1);
        stats.curMotion = cvCountNonZero(currentDiff.getCvImage()) * 1.0f/(size.x * size.y);
        stats.avgMotion *= 1-f;
        stats.avgMotion += stats.curMotion * f;
        stats.avgMotionNorm = ofNormalize(stats.avgMotion, 0, settings.diff.avgMotionMax);
        if(stats.avgMotion <= settings.noMotionThreshold * settings.noMotionThreshold) {
            if(stats.hasMotion) {
                stats.hasMotion    = false;
                stats.idleStartTime = nowTime;
                stats.motionFrames = stats.motionTime = 0;
            }
            stats.idleTime = nowTime - stats.idleStartTime;
            stats.idleFrames++;
        } else {
            if(!stats.hasMotion) {
                stats.hasMotion = true;
                stats.motionStartTime = nowTime;
                stats.idleFrames = stats.idleTime = 0;
            }
            stats.motionTime = nowTime - stats.motionStartTime;
            stats.motionFrames++;
        }

        // autosave background
        if(settings.autoBGSaveTime > 0 && (stats.idleTime > settings.autoBGSaveTime) && (nowTime - stats.lastTimeBGWasSaved > settings.autoBGSaveTime)) settings.learnBG = true;

        if(settings.learnThreshold) {
            if(isLearning==false) {
                learningSound.play();
                isLearning = true;
                learnStartTime = ofGetElapsedTimef();
            }

            if(stats.curMotion>0) settings.diff.threshold ++;
            else settings.diff.threshold --;

            if(settings.diff.threshold<0) settings.diff.threshold = 0;

            //                settings.bottomThreshold = settings.diff.threshold;

            stats.lastTimeBGWasSaved = nowTime;
            printf("Tracker::update() - learnThreshold\n");

            if(ofGetElapsedTimef() - learnStartTime > settings.learnBGTime) {
                printf("Tracker::update() - stopping learnThreshold\n");
                isLearning = false;
                settings.learnThreshold = false;
                learningSound.stop();
                doneSound.play();
                settings.diff.threshold = (settings.diff.threshold + 2) * 1.1;
                //                    settings.bottomThreshold = settings.diff.threshold;
                gui.saveToXML();
            }
            settings.learnBG = true;

        }

        if(settings.learnBG) {
            settings.learnBG = false;
            cleanPlate = currentGreyImage;
            static ofImage imgCleanPlate;
            imgCleanPlate.setFromPixels(cleanPlate.getPixels());
            //imgCleanPlate.setFromPixels(cleanPlate.getPixels(), cleanPlate.getWidth(), cleanPlate.getHeight(), OF_IMAGE_GRAYSCALE);
            imgCleanPlate.save("settings/Tracker/cleanplate.png");
        }


        if(settings.subtractBG) processedGreyImage.absDiff(processedCleanPlate, processedGreyImage);

        if(settings.lessThanBG) cvCmp(processedGreyImage.getCvImage(), processedCleanPlate.getCvImage(), processedGreyImage.getCvImage(), CV_CMP_GT);

        if(settings.greaterThanBG) cvCmp(processedGreyImage.getCvImage(), processedCleanPlate.getCvImage(), processedGreyImage.getCvImage(), CV_CMP_LT);

        if(settings.pre.brightness) cvScale(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.pre.brightness * settings.pre.brightness);    // brightness 0...1
        if(settings.pre.bottomThreshold) cvThreshold(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.pre.bottomThreshold, 255, CV_THRESH_TOZERO);
        if(settings.pre.binaryThreshold) cvThreshold(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.pre.binaryThreshold, 255, CV_THRESH_BINARY);

        if(settings.learnBottom) {
            if(isLearning==false) {
                learningSound.play();
                isLearning = true;
                learnStartTime = ofGetElapsedTimef();
            }

            //                if(processedGreyImage.countNonZeroInRegion(0, 0, width, height) > 0) settings.bottomThreshold++;
            if(cvCountNonZero(processedGreyImage.getCvImage())) settings.pre.bottomThreshold++;
            else settings.pre.bottomThreshold--;
            printf("Tracker::update() - learnBottom\n");

            if(ofGetElapsedTimef() - learnStartTime > settings.learnBGTime) {
                printf("Tracker::update() - stopping learnBottom\n");
                isLearning = false;
                settings.learnBottom = false;
                learningSound.stop();
                doneSound.play();
                settings.pre.bottomThreshold = (settings.pre.bottomThreshold + 2) * 1.1;
                gui.saveToXML();
            }
        }


        if(settings.pre.topThreshold) cvThreshold(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.pre.topThreshold, 255, CV_THRESH_TRUNC);
        //                if(settings.pre.normalize) cvEqualizeHist(processedGreyImage.getCvImage(), processedGreyImage.getCvImage());
        //            cvNormalize(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), 255+10*settings.topThreshold, -10*settings.bottomThreshold, CV_MINMAX, NULL);    // the 10* is just an empirical hack
        if(settings.pre.adaptiveThreshold) {
            cvAdaptiveThreshold(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY, 2*settings.pre.adaptiveBlockSize+1, settings.pre.adaptiveThreshold);
            processedGreyImage.invert();
        }

        if(settings.post.invert) {
            processedGreyImage.invert();
            //                currentGreyImage.invert();
        }

        if(settings.preBlobsBlur) cvSmooth( processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), CV_BLUR , settings.preBlobsBlur*2+1);

        if(settings.dilateBeforeErode) {
            if(settings.dilateAmount) cvDilate(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), 0, settings.dilateAmount);
            if(settings.erodeAmount) cvErode(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), 0, settings.erodeAmount);
        } else {
            if(settings.erodeAmount) cvErode(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), 0, settings.erodeAmount);
            if(settings.dilateAmount) cvDilate(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), 0, settings.dilateAmount);
        }

        if(settings.doLaplace) {
            //                cvLaplace(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), 3);

            static IplImage *dx = 0;
            if(dx==0) dx = cvCreateImage(cvGetSize(processedGreyImage.getCvImage()),IPL_DEPTH_16S,1);

            cvLaplace(processedGreyImage.getCvImage(), dx, 5);

            /* Convert signed to unsigned 8*/
            cvConvertScaleAbs( dx , processedGreyImage.getCvImage(), 1, 0);
        }

        if(settings.sobel.enabled) {
            cvSobel(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.sobel.order, settings.sobel.order, settings.sobel.blockSize*2+1);
        }


        if(settings.canny.enabled) {
            cvCanny(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.canny.lowThreshold, settings.canny.highThreshold, settings.canny.blockSize*2+1);
        }


        if(settings.findBlobsOnCurrent || settings.findBlobsOnDiff || settings.findBlobsOnAccDiff) {
            ofxCvGrayscaleImage *tempImage;
            if(settings.findBlobsOnDiff) {
                tempImage = &currentDiff;
            } else if(settings.findBlobsOnAccDiff) {
                tempImage = &accDiff;
            } else {
                tempImage = &processedGreyImage;
            }
            contourFinder.findContours(*tempImage, settings.minBlobSize * settings.minBlobSize * size.x * size.y, settings.maxBlobSize * settings.maxBlobSize * size.x * size.y, settings.maxNumBlobs, settings.findHoles, settings.useApproximation);

            //
            //                stats.motionCenter.set(0, 0);
            //                float totalArea = 0;
            //                int numBlobs = contourFinder.blobs.size();
            //
            //                for(int b = 0; b < numBlobs; b++) {
            //                    ofxCvBlob &blob = contourFinder.blobs[b];
            //                    stats.motionCenter += blob.centroid * blob.area;
            //                    totalArea += blob.area;
            //                }

            if(settings.blurContourKernel>0) {
                int numBlobs = contourFinder.blobs.size();
                vector<ofDefaultVec3> newContour;

                int numSamples = 2 * settings.blurContourKernel + 1;
                float avgInv = 1.0f/numSamples;

                ofDefaultVec3 average;

                for(int b = 0; b < numBlobs; b++) {
                    ofxCvBlob &blob = contourFinder.blobs[b];
                    int numPoints = blob.pts.size();
                    newContour = blob.pts;

                    for(int p=0; p<numPoints; p++) {
                        blob.pts[p] = ofDefaultVec3(0, 0, 0);
                        for(int i=0; i<numSamples; i++) {
                            blob.pts[p] += newContour[mod(p + i - settings.blurContourKernel, numPoints)];
                        }
                        blob.pts[p] *= avgInv;
                    }
                }
            }

            if(settings.simplifyContour>0) {
                vector<ofDefaultVec3> newContour;

                int skipPoints = settings.simplifyContour + 1;
                int numBlobs = contourFinder.blobs.size();
                for(int b = 0; b < numBlobs; b++) {
                    ofxCvBlob &blob = contourFinder.blobs[b];
                    int numPoints = blob.pts.size();
                    newContour.clear();
                    newContour.reserve(numPoints / skipPoints);
                    for(int p=0; p<numPoints; p+= skipPoints) {
                        newContour.push_back(blob.pts[p]);
                    }
                    //contourFinder.blobs[b].pts.clear();
                    blob.pts = newContour;
                    blob.nPts = newContour.size();
                }

            }
        }

        if(settings.post.blur) cvSmooth(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), CV_BLUR , settings.post.blur*2+1);
        if(settings.post.openAmount) cvMorphologyEx(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), NULL, NULL, CV_MOP_OPEN, settings.post.openAmount);
        if(settings.post.closeAmount) cvMorphologyEx(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), NULL, NULL, CV_MOP_CLOSE, settings.post.closeAmount);
        if(settings.post.bottomThreshold) cvThreshold(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.post.bottomThreshold, 255, CV_THRESH_TOZERO);
        if(settings.post.topThreshold) cvThreshold(processedGreyImage.getCvImage(), processedGreyImage.getCvImage(), settings.post.topThreshold, 255, CV_THRESH_TRUNC);
        if(settings.post.normalize) cvEqualizeHist(processedGreyImage.getCvImage(), processedGreyImage.getCvImage());


        if(settings.applyMask)    {
            //                maskedImage->set(0);
            //                cvCopy(resizedImage->getCvImage(), maskedImage->getCvImage(), processedGreyImage.getCvImage());
            //                if(inputIsColor) {
            //                    cvCvtColor(processedGreyImage.getCvImage(), maskedImage->getCvImage(), CV_GRAY2BGR);
            //                    cvMul(maskedImage->getCvImage(), resizedImage->getCvImage(), maskedImage->getCvImage(), 1.0f/255.0f);
            //                } else {
            //                    cvMul(resizedImage->getCvImage(), processedGreyImage.getCvImage(), maskedImage->getCvImage(), 1.0f/255.0f);
            //                }
            cvMul(currentGreyImage.getCvImage(), processedGreyImage.getCvImage(), maskedImage->getCvImage(), 1.0f/255.0f);
            if(settings.doFloodfill) {
                cvWatershed(maskedImage->getCvImage(), processedGreyImage.getCvImage());
            }
            maskedImage->flagImageChanged();
        }

        opticalFlow.calc(opFlowInput2, opFlowInput1);

        // do custom stuff
        //            customUpdate();

        /*
             //save camera grab
             static int lastCameraGrabTime = 0;
             static int cameraGrabSeed = 0;
             if(settings.saveCameraImageTime && nowTime - lastCameraGrabTime > settings.saveCameraImageTime && stats.idleTime < settings.saveCameraImageTime) {
             //                if(cameraGrabSeed == 0) cameraGrabSeed = ofRandom(0,
             static ofImage imgCameraGrab;
             imgCameraGrab.setFromPixels(vidGrabber.getPixels(), vidGrabber.getWidth(), vidGrabber.getHeight(), OF_IMAGE_COLOR);
             imgCameraGrab.saveImage("camgrabs/camgrab_" + ofToString(ofGetYear(), 4) + ofToString(ofGetMonth(), 4) + ofToString(ofGetDay(), 4) + "_" + ofToString(ofGetFrameNum()) + ".png");
             lastCameraGrabTime = nowTime;
             }
             */

    } else {
        frameCounter ++;
    }
    //        }

    //#if MSA_VIDEO_INPUT == MSA_USE_PTGREY
    //    vidGrabber.doneWithCurrentFrame();
    //#endif
}



//bool MotionTracker::hasBeenIdleFor(float secs) {
//    return stats.idleTime > secs;
//}

bool MotionTracker::isIdle() {
    return settings.idleTriggerTime && stats.idleTime > settings.idleTriggerTime;
}


void MotionTracker::setupUI() {
    gui.saveToXML();        // save all current values so they don't get lost when we load later

    gui.addPage("TRACKER OPTIONS").setXMLName("settings/Tracker/TrackerOptions.xml");
    gui.addToggle("enabled", settings.enabled);
    gui.addToggle("reinit", settings.reinit);
    gui.addTitle("TRANSFORM");
    gui.addToggle("flipVideoX", settings.transform.flipVideoX);
    gui.addToggle("flipVideoY", settings.transform.flipVideoY);
    gui.addToggle("rotateVideo90", settings.transform.rotateVideo90);
    gui.addSlider("offsetX", settings.transform.offset.x, -1, 1);
    gui.addSlider("offsetY", settings.transform.offset.y, -1, 1);
    gui.addSlider("scaleX", settings.transform.scale.x, 0.25, 4);
    gui.addSlider("scaleY", settings.transform.scale.y, 0.25, 4);

    gui.addSlider("scaleDown", settings.scaleDown, 1, 8);
    gui.addSlider("LearnBGTime", settings.learnBGTime, 0, 10);
    gui.addSlider("autoBGSaveTime", settings.autoBGSaveTime, 0, 60*5);
    gui.addSlider("idleTriggerTime", settings.idleTriggerTime, 0, 60*5);
    gui.addSlider("noMotionThreshold", settings.noMotionThreshold, 0.001, 0.01);
    gui.addSlider("saveCameraImageTime", settings.saveCameraImageTime, 0, 60);
    gui.addSlider("averageFrames", settings.averageFrames, 0, 120);
    gui.addToggle("doDrawCurrent", settings.doDrawCurrent);
    gui.addToggle("doDrawProcessed", settings.doDrawProcessed);
    gui.addToggle("doDrawMasked", settings.doDrawMasked);
    gui.addToggle("doDrawRaw", settings.doDrawRaw);
    gui.addToggle("doDrawResized", settings.doDrawResized);
    gui.addToggle("doDrawDiff", settings.doDrawDiff);
    gui.addToggle("doDrawDiffAcc", settings.doDrawDiffAcc);
    gui.addToggle("doDrawContours", settings.doDrawContours);
    gui.addToggle("doDrawMotionCentroid", settings.doDrawMotionCentroid);
    gui.addSlider("alpha", settings.alpha, 0, 1);

    gui.addPage("TRACKER CONFIG").setXMLName("settings/Tracker/TrackerConfig.xml");

    gui.addTitle("PRE");
    gui.addSlider("pre.blur", settings.pre.blur, 0, 15);
    gui.addSlider("pre.brightness", settings.pre.brightness, 0, 2);
    gui.addSlider("pre.bottomThreshold", settings.pre.bottomThreshold, 0, 255);
    gui.addSlider("pre.topThreshold", settings.pre.topThreshold, 0, 255);
    gui.addSlider("pre.binaryThreshold", settings.pre.binaryThreshold, 0, 255);
    gui.addSlider("pre.adaptiveThreshold", settings.pre.adaptiveThreshold, 0, 10);
    gui.addSlider("pre.adaptiveBlockSize", settings.pre.adaptiveBlockSize, 1, 10);

    gui.addTitle("THRESHOLD");
    gui.addToggle("Learn BG", settings.learnBG);
    gui.addToggle("learnThreshold", settings.learnThreshold);
    gui.addToggle("Motion detected", stats.hasMotion);

    gui.addTitle("DIFF");
    gui.addToggle("diff.enabled", settings.diff.enabled);
    gui.addSlider("diff.threshold", settings.diff.threshold, 0, 255);
    gui.addSlider("diff.blur1", settings.diff.blur1, 0, 15);
    gui.addSlider("diff.openAmount", settings.diff.openAmount, 0, 10);
    gui.addSlider("diff.closeAmount", settings.diff.closeAmount, 0, 10);
    gui.addSlider("diff.blur2", settings.diff.blur2, 0, 15);
    gui.addSlider("diff.avgMotionMax", settings.diff.avgMotionMax, 0, 1);

    gui.addTitle("ACC DIFF");
    gui.addToggle("accDiff.enabled", settings.accDiff.enabled);
    gui.addSlider("accDiff.blur1", settings.accDiff.blur1, 0, 15);
    gui.addSlider("accDiff.newWeight", settings.accDiff.newWeight, 0, 1);
    gui.addSlider("accDiff.oldWeight", settings.accDiff.oldWeight, 0, 1);
    gui.addSlider("accDiff.blur2", settings.accDiff.blur2, 0, 15);
    gui.addSlider("accDiff.bottomThreshold", settings.accDiff.bottomThreshold, 0, 255);
    gui.addSlider("accDiff.adaptiveThreshold", settings.accDiff.adaptiveThreshold, 0, 10);
    gui.addSlider("accDiff.adaptiveBlockSize", settings.accDiff.adaptiveBlockSize, 1, 10);


    gui.addTitle("OPTICAL FLOW");
    gui.addToggle("opFlow.enabled", opticalFlow.enabled);
    gui.addSlider("opFlow.scaleDown", opticalFlow.scaleDown, 1, 12);
    gui.addSlider("opFlow.blockSize", opticalFlow.blockSize, 1, 7);
    gui.addSlider("opFlow.iterations", opticalFlow.iterations, 1, 20);
    gui.addSlider("opFlow.pyr_scale", opticalFlow.pyr_scale, 0, 1);
    gui.addSlider("opFlow.pyr_levels", opticalFlow.pyr_levels, 1, 20);
    gui.addSlider("opFlow.blur", opticalFlow.blur, 0, 15);
    gui.addSlider("opFlow.maxSpeed", opticalFlow.maxSpeed, 0, 1);
    gui.addSlider("opFlow.drawThreshold2", opticalFlow.drawThreshold2, 0, 50);
    gui.addSlider("opFlow.drawStep", opticalFlow.drawStep, 1, 10);

    gui.addTitle("CANNY");
    gui.addToggle("canny.enabled", settings.canny.enabled);
    gui.addSlider("canny.lowThreshold", settings.canny.lowThreshold, 0, 255);
    gui.addSlider("canny.highThreshold", settings.canny.highThreshold, 0, 255);
    gui.addSlider("canny.blockSize", settings.canny.blockSize, 1, 3);

    gui.addTitle("SOBEL");
    gui.addToggle("sobel.enabled", settings.sobel.enabled);
    gui.addSlider("sobel.order", settings.sobel.order, 1, 2);
    gui.addSlider("sobel.blockSize", settings.sobel.blockSize, 1, 3);


    gui.addTitle("POST");
    gui.addToggle("doLaplace", settings.doLaplace);
    gui.addSlider("post.blur", settings.post.blur, 0, 15);
    gui.addSlider("post.bottomThreshold", settings.post.bottomThreshold, 0, 255);
    gui.addSlider("post.topThreshold", settings.post.topThreshold, 0, 255);
    gui.addToggle("post.normalize", settings.post.normalize);
    gui.addToggle("post.invert", settings.post.invert);
    gui.addSlider("post.openAmount", settings.post.openAmount, 0, 10);
    gui.addSlider("post.closeAmount", settings.post.closeAmount, 0, 10);



    //        gui.addPage("SEGMENTATION").setXMLName("settings/Tracker/segmentation.xml");
    //        gui.addSlider("preBlobsBlur", settings.preBlobsBlur, 0, 15);
    //        gui.addToggle("subtractBG", settings.subtractBG);
    //        gui.addToggle("lessThanBG", settings.lessThanBG);
    //        gui.addToggle("greaterThanBG", settings.greaterThanBG);
    //        gui.addToggle("applyMask", settings.applyMask);
    //        gui.addToggle("doFloodfill", settings.doFloodfill);
    //        gui.addSlider("erodeAmount", settings.erodeAmount, 0, 10);
    //        gui.addSlider("dilateAmount", settings.dilateAmount, 0, 10);
    //        gui.addToggle("dilateBeforeErode", settings.dilateBeforeErode);

    //        gui.addTitle("BLOBS");
    //        gui.addToggle("findBlobsOnCurrent", settings.findBlobsOnCurrent);
    //        gui.addToggle("findBlobsOnDiff", settings.findBlobsOnDiff);
    //        gui.addToggle("findBlobsOnAccDiff", settings.findBlobsOnAccDiff);
    //        gui.addToggle("findHoles", settings.findHoles);
    //        gui.addToggle("useApproximation", settings.useApproximation);
    //        gui.addSlider("minBlobSize", settings.minBlobSize, 0, 1);
    //        gui.addSlider("maxBlobSize", settings.maxBlobSize, 0, 1);
    //        gui.addSlider("maxNumBlobs", settings.maxNumBlobs, 0, 100);
    //        gui.addSlider("blurContourKernel", settings.blurContourKernel, 0, 100);
    //        gui.addSlider("simplifyContour", settings.simplifyContour, 0, 50);

    gui.addPage("STATS").setXMLName("");
    gui.addSlider("averageFrames", settings.averageFrames, 0, 120);
    gui.addSlider("noMotionThreshold", settings.noMotionThreshold, 0.001, 0.01);

    gui.addToggle("hasMotion", stats.hasMotion);
    gui.addSlider("avgVel.x", stats.avgVel.x, -1, 1);
    gui.addSlider("avgVel.y", stats.avgVel.y, -1, 1);
    gui.addSlider("avgSpeed2", stats.avgSpeed2, 0, 1);
    gui.addSlider("curMotion", stats.curMotion, 0, 1);
    gui.addSlider("avgMotion", stats.avgMotion, 0, 1);
    gui.addSlider("avgMotionNorm", stats.avgMotionNorm, 0, 1);
    gui.addSlider("motionCenter.x", stats.motionCenter.x, 0, 1);
    gui.addSlider("motionCenter.y", stats.motionCenter.y, 0, 1);
    gui.addSlider("idleTime", stats.idleTime, 0, 1);
    gui.addSlider("idleFrames", stats.idleFrames, 0, 1);
    gui.addSlider("motionTime", stats.motionTime, 0, 1);
    gui.addSlider("motionFrames", stats.motionFrames, 0, 1);
    gui.addSlider("captureFPS", stats.captureFPS, 0, 120);



    //guiPages.config = &
    //        gui.addPage("WARP").setXMLName("settings/Tracker/TrackerWarp.xml");
    //        gui.addToggle("doWarpInput", settings.doWarpInput);


    gui.loadFromXML();

		memset(&stats, sizeof(stats), 0);

    if(roiSettings.roi.x1  >= size.x - 1) roiSettings.roi.x1 = 0;
    if(roiSettings.roi.y1  >= size.y -1) roiSettings.roi.y1 = 0;
    if(roiSettings.roi.x2 == 0) roiSettings.roi.x2 = size.x;
    if(roiSettings.roi.y2 == 0) roiSettings.roi.y2 = size.y;

    roiSettings.showRoi = false;
    roiSettings.setRoi = true;

    gui.saveToXML();
}


void MotionTracker::drawContour(float x, float y, float w, float h) const {
    ofPushStyle();
    ofSetRectMode(OF_RECTMODE_CORNER);
    float scalex = 0.0f;
    float scaley = 0.0f;
    if( size.x != 0 ) { scalex = w/contourFinder.getWidth(); } else { scalex = 1.0f; }
    if( size.y != 0 ) { scaley = h/contourFinder.getHeight(); } else { scaley = 1.0f; }

    ofSetHexColor(0xFFFFFF);
    ofSetLineWidth(3);

    if(roiSettings.useRoi) {
        x += roiSettings.roi.x1 * scalex;
        y += roiSettings.roi.y1 * scaley;
    }

    ofNoFill();
    ofPushMatrix();
    ofTranslate( x, y, 0.0 );
    ofScale( scalex, scaley, 0.0 );

    for( int i=0; i<(int)contourFinder.blobs.size(); i++ ) {
        ofBeginShape();
        for( int j=0; j<contourFinder.blobs[i].nPts; j++ ) {
            ofVertex( contourFinder.blobs[i].pts[j].x, contourFinder.blobs[i].pts[j].y );
        }
        ofEndShape();

    }
    ofPopMatrix();
    ofFill();
    ofPopStyle();
}


void MotionTracker::drawCameraIcon(int w) const {
    ofSetColor(255);
    ofDisableAlphaBlending();
    imgCameraIcon.draw(w - imgCameraIcon.getWidth()-50, 50);
}



void MotionTracker::startDraw(float x, float y, float w, float h, bool flipX, bool flipY) const {
    Vec2f scale(flipX ? -1 : 1, flipY ? -1 : 1);

    ofSetRectMode(OF_RECTMODE_CENTER);
    ofPushMatrix();
    ofTranslate(x + w/2, y + h/2);
    scale *= settings.transform.scale;
    if(settings.transform.rotateVideo90) {
        ofRotate(90, 0, 0, 1);
        scale.y *= -1;
        ofScale(scale.x * h, scale.y * w);
    } else {
        ofTranslate(settings.transform.offset.x * w, settings.transform.offset.y * h);
        ofScale(scale.x * w, scale.y * h);
    }
    //        ofTranslate(-w/2, -h/2);
}

void MotionTracker::endDraw() const {
    ofPopMatrix();
    ofSetRectMode(OF_RECTMODE_CORNER);
}

void MotionTracker::draw(const ofBaseImage &img, float x, float y, float w, float h, bool flipX, bool flipY, bool showRoi) const {
    startDraw(x, y, w, h, flipX, flipY);
    img.draw(0, 0, 1, 1);
    endDraw();

    if(showRoi) {
        ofNoFill();
        ofPushMatrix();
        ofTranslate(x, y, 0);
        ofScale(w*invSize.x, h*invSize.y, 1);
        ofDrawRectangle(roiSettings.roi.x1, roiSettings.roi.y1, roiSettings.roi.width(), roiSettings.roi.height());
        ofPopMatrix();
        ofFill();
    }

}


void MotionTracker::draw(float x, float y, float w, float h) const {
    if(!isReady()) return;

    ofDisableDepthTest();

    if(settings.doDrawProcessed && settings.alpha) {
        ofEnableAlphaBlending();
        int b = 255*settings.alpha;
        ofSetColor(b, b, b);
        draw(processedGreyImage, x, y, w, h);
        drawCameraIcon(w);
    }

    if(settings.doDrawCurrent && settings.alpha) {
        ofSetColor(255);
        ofDisableAlphaBlending();
        draw(currentGreyImage, x, y, w, h);
        drawCameraIcon(w);
    }

    if(settings.doDrawMasked && settings.alpha) {
        ofEnableAlphaBlending();
        ofSetColor(255, 255*settings.alpha);
        draw(*maskedImage, x, y, w, h);
    }

    if(settings.doDrawRaw || isLearning || roiSettings.showRoi) {
        ofSetColor(255);
        ofDisableAlphaBlending();
        draw(*captureImage, x, y, w, h, settings.transform.flipVideoX, settings.transform.flipVideoY, roiSettings.showRoi);
        drawCameraIcon(w);
    }

    if(settings.doDrawResized && settings.alpha) {
        ofSetColor(255);
        ofDisableAlphaBlending();
        draw(*resizedImage, x, y, w, h, settings.transform.flipVideoX, settings.transform.flipVideoY, roiSettings.showRoi);
        drawCameraIcon(w);
    }

    if(settings.doDrawDiff && settings.alpha) {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        int b = 255*settings.alpha;
        ofSetColor(b, b, b);
        draw(currentDiff, x, y, w, h);
    }

    if(settings.doDrawDiffAcc && settings.alpha) {
        ofEnableAlphaBlending();
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        draw(accDiff, x, y, w, h);
    }


    if(isLearning) {
        ofEnableAlphaBlending();
        ofSetColor(255, 100);
        imgTrackerLearning.draw(w/2 - imgTrackerLearning.getWidth()/2, h/2 - imgTrackerLearning.getHeight()/2);

        ofDisableAlphaBlending();
        ofDrawBitmapString("curMotion: " + ofToString(stats.curMotion, 7)
                           + " | diff.threshold: " + ofToString(settings.diff.threshold, 7)
                           + " | bottomThreshold: " + ofToString(settings.pre.bottomThreshold, 7)
                           , w/2 - imgTrackerLearning.getWidth()/2 + 40, h/2 + imgTrackerLearning.getHeight()/2 - 40);
    }

    if(settings.doDrawContours) {
        startDraw(x, y, w, h, settings.transform.flipVideoX, settings.transform.flipVideoY);
        drawContour(0, 0, 1, 1);
        endDraw();
    }

    if(settings.doDrawMotionCentroid) {
        ofSetColor(255, 255, 255);
        ofDrawCircle(stats.motionCenter.x * w, stats.motionCenter.y * h, 50);
    }
}


void MotionTracker::keyPressed(ofKeyEventArgs &e) {
    if(!(e.hasModifier(OF_KEY_CONTROL))) return;
    int key = e.keycode;
    switch(key) {
    //            case 'A': settings.learnBottom ^= true; if(settings.learnBottom) settings.pre.bottomThreshold = 0; break;
    case 'A': settings.learnThreshold ^= true; break;
    case 'B': settings.learnBG = true; break;
        //            case 'C': settings.doDrawRaw ^= true;    settings.doDrawProcessed = false; settings.doDrawDiff = false; settings.doDrawCurrent = false; break;
    case 'C': settings.doDrawCurrent ^= true; settings.doDrawProcessed = false; settings.doDrawRaw = false; break;
    case 'D': settings.doDrawDiff ^= true; break;
    }
}

// transform coordinates to normalized coordinates applying offset and scale
static Vec2f posTransformedToNorm(Vec2f pos, Vec2f dim, Vec2f offset, Vec2f scale) {
    Vec2f ret(pos/dim); // normalized

    ret -= Vec2f(0.5);
    ret *= scale;
    ret += Vec2f(0.5);

    ret += offset;

    return ret;
}

Vec2f MotionTracker::camToWorldNorm(Vec2f camPos) const {
    return posTransformedToNorm(camPos, Vec2f(getWidth(), getHeight()), settings.transform.offset, settings.transform.scale);
}

}
