//
// OpticalFlowDense
//
// Started from ofxOpticalFlowLK by Takashi Maekawa, 2008 <takachin@generative.info>
// Updates 2009, 2010 by Memo Akten <http://www.msavisuals.com>
//     All rights reserved.
//     This is free software with ABSOLUTELY NO WARRANTY.
//
// You can redistribute it and/or modify it under the terms of 
// the GNU Lesser General Public License.
//

#pragma once

#include "MSACore.h"
#include "ofMain.h"
#include "ofxCvConstants.h"
#include "ofxCvGrayscaleImage.h"

namespace msa {
	
	class OpticalFlowDense : public ofBaseDraws {
	public:
		
		bool		enabled;
		int			scaleDown;
		int			blockSize;						// blocksize for optical flow
		int			blur;
		float		maxSpeed;
		float		drawThreshold2;
		int			drawStep;
        
        // new for farneback
        float       pyr_scale = 0.5;  // 0...1
        int         pyr_levels = 1;
        int         iterations = 1;
        int         poly_n = 5; // or 7
        double      poly_sigma = 1.1;   // 1.1 for n==5, 1.5 for n==7
        
		
		Vec2f		*velArray;
		

		
		OpticalFlowDense(void);
		~OpticalFlowDense(void);
		
		void allocate(int _w, int _h);
		
		void calc(ofxCvGrayscaleImage& pastImage, ofxCvGrayscaleImage& currentImage);
		
		void reset();
		void draw() const;
        void draw(float x, float y) const override;
        void draw(float x, float y, float w, float h) const override;
		
		void setROI(int x, int y, int w, int h);
		void resetROI();
		
		Vec2f getVelocityAt(int i, int j) const {
			return velArray[(int)(i + j * size.x)];
		}
		
		Vec2f getVelocityAt(Vec2f pos) const {
			return getVelocityAt(pos.x, pos.y);
		}
		
		
		float getWidth() const override {
			return roiRect.width;
		}	
		
		float getHeight() const override {
			return roiRect.height;
		};
		
		float getInvWidth() const {
			return invSize.x;
		}
		
		float getInvHeight() const {
			return invSize.y;
		}
		
		const Vec2f& getSize() const {
			return size;
		}
		
		const Vec2f& getInvSize() const {
			return invSize;
		}
		
	protected:
		
		Vec2f size;
		Vec2f invSize;
		
		ofRectangle roiRect;
		
		void destroy();
		
        //cv::Mat cvFlow;
        IplImage* cvFlow;
		
	};
}