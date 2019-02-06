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

#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "MSAOpticalFlowDense.h"

namespace msa {
	
	struct RoiInfo {
		int x1;
		int y1;
		int x2;
		int y2;
		int width() const { return x2 - x1; }
		int height() const { return y2 - y1; }
	};
	
	class MotionTracker {
	public:
		int						videoFrame;
		
		ofxCvImage				*captureImage;				// raw captured image (can be color or greyscale)
		ofxCvImage				*resizedImage;
		ofxCvImage				*maskedImage;
		ofxCvGrayscaleImage 	cleanPlate;					// background plate
		ofxCvGrayscaleImage 	processedCleanPlate;		// background plate but blurred etc. if need be
		ofxCvGrayscaleImage 	currentGreyImage;			// greyscale version of now
		ofxCvGrayscaleImage 	processedGreyImage;			// greyscale version of now, but processed etc.
		ofxCvGrayscaleImage 	previousGreyImage;			// previous frame
		ofxCvGrayscaleImage 	currentDiff;				// difference between last 2 frames
		ofxCvGrayscaleImage		accDiff;					// accumulated diff
		ofxCvContourFinder		contourFinder;
		
		ofxCvGrayscaleImage 	opFlowInput1;				// greyscale version of now
		ofxCvGrayscaleImage 	opFlowInput2;				// previous frame
		OpticalFlowDense		opticalFlow;
		
//		float					opflowMultWidth;
//		float					opflowMultHeight;
		float					opticalFlowMult;
		
		Vec2f	size;
		Vec2f	invSize;

		
		struct {
			RoiInfo			roi;
			bool			showRoi;
			bool			useRoi;
			bool			setRoi;
			bool			restart;
		} roiSettings;
		
		struct {
			bool			enabled;
			int				scaleDown;
			bool			subtractBG;
			bool			lessThanBG;
			bool			greaterThanBG;
			bool			applyMask;
			bool			doFloodfill;
			float			autoBGSaveTime;					// number of seconds of no motion to wait before taking new background capture
			int				idleTriggerTime;				// number of seconds before switching to idle mode
			float			noMotionThreshold;			// trigger idle when avgMotion falls below this
			int				saveCameraImageTime;			// number of seconds before saving camera image
			
			bool			findBlobsOnCurrent;
			bool			findBlobsOnDiff;
			bool			findBlobsOnAccDiff;
			
			float			alpha;
			bool            doDrawCurrent;
			bool			doDrawProcessed;
			bool			doDrawMasked;
			bool			doDrawRaw;
			bool			doDrawResized;
			bool			doDrawDiff;
			bool			doDrawDiffAcc;
			bool            doDrawContours;
			bool			doDrawMotionCentroid;
			
			bool			learnBG;
			int				learnBGTime;
			bool			learnBottom;
			bool			learnThreshold;
			
			struct {
				bool			flipVideoX;
				bool			flipVideoY;
				bool			rotateVideo90;
				Vec2f			offset;
				Vec2f			scale;
			} transform;
			
			struct {
				int				blur;						// blurring before differencing
				float			brightness;
				float			bottomThreshold;				// cutoff point for bottom
				float			topThreshold;					// cutoff point for top
				float			binaryThreshold;				// cuts off to black below, white above
				float			adaptiveThreshold;
				int				adaptiveBlockSize;				// block size for adaptive threshold
			} pre;
			
			struct {
				bool		enabled;
				float		threshold;
				int			blur1;							// blur before processing
				int			openAmount;
				int			closeAmount;
				int			blur2;							// blur after processing
            float        avgMotionMax;                   // for normalising stats.avgMotion
			} diff;
			
			struct {
				bool		enabled;
				int			blur1;							// blur before processing
				float		newWeight;
				float		oldWeight;
				int			blur2;							// blur after processing
				float		bottomThreshold;				// cutoff point for bottom
				float		adaptiveThreshold;
				int			adaptiveBlockSize;				// block size for adaptive threshold
			} accDiff;
			
			struct {
				bool		enabled;
				float		lowThreshold;
				float		highThreshold;
				int			blockSize;
			} canny;
			
			struct {
				bool		enabled;
				int			order;
				int			blockSize;
			} sobel;
			
			struct {
				int			blur;
				float		bottomThreshold;				// cutoff point for bottom
				float		topThreshold;					// cutoff point for top
				bool		normalize;
				bool		invert;
				int			openAmount;
				int			closeAmount;
			} post;
			
			int				preBlobsBlur;
			
			bool			findHoles;
			bool			useApproximation;
			float			minBlobSize;
			float			maxBlobSize;
			int				maxNumBlobs;
			int				erodeAmount;
			int				dilateAmount;
			bool			dilateBeforeErode;
			int             simplifyContour;
			int				blurContourKernel;
			
			bool			doLaplace;
			
			int				averageFrames;					// number of frames to to average for motion detection
			
			bool			doWarpInput;
			ofPoint			inputPoints[4];
			
		} settings;
		
		
		struct {
			ofPoint			avgVel;
			float			avgSpeed2;
			float			curMotion;			// how much motion there is in the current frame
        float            avgMotion;            // how much motion there is over the averaged frames frame (ratio 0...1, but in reality tiny)
        float            avgMotionNorm;        // normalised 0...1 with avgMotionMax
			int				idleFrames;
			int				motionFrames;
			float			idleTime;
			float			motionTime;
			float			captureFPS;
			ofPoint			motionCenter;		// centroid of all moving points (normalized)
			
			float			motionStartTime;
			float			idleStartTime;
			float			lastTimeBGWasSaved;
			float			lastTimeHadFrame;
			
			bool			hasMotion;
		} stats;
		
		MotionTracker();
		//	~MotionTracker();
		
		bool isIdle();
		
		void setup();
		void update(ofPixelsRef inputImage);
		void reset();
		
		void drawContour(float x, float y, float w, float h) const;
		void draw(float x, float y, float w, float h) const;
		

		// convert coordinates in camera space, to normalized world coordinates
		// takes into account offset, scale, and rotation
		// doesnt take into account flip, because that acts on pixels anyhow
		Vec2f camToWorldNorm(Vec2f camPos) const;
		
		float getWidth() const {
			return size.x;
		}
		
		float getHeight() const {
			return size.y;
		}
		
        bool isReady() const {
            return captureImage != NULL;
        }
        
        void			allocateTextures(ofPixelsRef inputImage);

	protected:
		ofImage			imgTrackerLearning;
		ofImage			imgCameraIcon;
		ofSoundPlayer	doneSound;
		ofSoundPlayer	learningSound;
		bool			isLearning;
		
		int				frameCounter;		// keep track of frames running
		float			learnStartTime;
		
		void			setupUI();
		void			keyPressed(ofKeyEventArgs &e);
		
		void			drawCameraIcon(int x) const;
		
		void			clear();
		
		void startDraw(float x, float y, float w, float h, bool flipX = false, bool flipY = false) const;
		void endDraw() const;
		void draw(const ofBaseImage &img, float x, float y, float w, float h, bool flipX = false, bool flipY = false, bool showRoi = false) const;

	};
    
    extern MotionTracker tracker;
}

