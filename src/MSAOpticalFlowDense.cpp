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

#include "MSAOpticalFlowDense.h"
#include "ofxOpenCv.h"

namespace msa {
	
	//#define		IX(i, j)		( (i) + (width *(j)) )
	
	OpticalFlowDense::OpticalFlowDense() {
		drawStep		= 4;
		drawThreshold2	= 10;
		maxSpeed		= 0;
		
        cvFlow          = 0;
		
		velArray		= 0;
	}
	
	OpticalFlowDense::~OpticalFlowDense() {
		destroy();
	}
	
	void OpticalFlowDense::reset() {
        if(cvFlow) cvSetZero(cvFlow);
		if(velArray) for(int i=0; i< size.x * size.y; i++) velArray[i] = Vec2f::zero();
	}
	
	void OpticalFlowDense::destroy() {
		DelArray(velArray);
        if(cvFlow) cvReleaseImage(&cvFlow);
	}
	
	
	void OpticalFlowDense::allocate(int w, int h){
		printf("OpticalFlowDense::allocate %i %i\n", w, h);
		destroy();
		
		size		= Vec2f(w, h);
		invSize		= Vec2f(1.0f/w, 1.0f/h);
		
        cvFlow = cvCreateImage( cvSize( w, h ), IPL_DEPTH_32F, 2 );
		
		velArray = new Vec2f[w * h];
		
		reset();
		
		resetROI();
	}
	
	void OpticalFlowDense::calc( ofxCvGrayscaleImage & pastImage, ofxCvGrayscaleImage & currentImage) {
		if(enabled == false) return;
		
		int bs = blockSize*2 + 1;
		
//		cvCalcOpticalFlowDense( pastImage.getCvImage(), currentImage.getCvImage(), cvSize( bs, bs), cvImageVel_x, cvImageVel_y );
        cvCalcOpticalFlowFarneback(pastImage.getCvImage(),
                                   currentImage.getCvImage(),
                                   cvFlow,
                                   pyr_scale,
                                   pyr_levels,
                                   bs,
                                   iterations,
                                   poly_n,
                                   poly_sigma,
                                   0);
                                   

		if(blur) {
			int blurSize = blur*2+1;
            cvSmooth(cvFlow, cvFlow, CV_BLUR , blurSize);
		}
		
		float ms = maxSpeed * size.y;
		
		int index = 0;
		for(int y=0; y< size.y; y++ ) {
            const float* f = (const float*)(cvFlow->imageData + cvFlow->widthStep*y);
			for(int x=0; x< size.x; x++ ) {
				velArray[index] = Vec2f(f[2*x], f[2*x+1]);
				if(ms) velArray[index].limit(ms);
				index++;
			}
		}
	}
	
	void OpticalFlowDense::draw() const {
		Vec2f v;
		for(int j = 0; j < getHeight(); j += drawStep) {
			for(int i = 0; i < getWidth(); i += drawStep){
				Vec2f v = getVelocityAt(i, j);
				if(v.lengthSquared() > drawThreshold2) {
//					printf("%f\n", v.lengthSquared());
					glBegin(GL_LINES);
					glColor3f(1, 1, 1); glVertex2f(i, j);
					glColor3f(1, 0, 0); glVertex2f(i + v.x*2, j + v.y*2);
					glEnd();
				}
			}
		}
	}
	
	void OpticalFlowDense::draw(float x, float y) const {
		glPushMatrix();
		glTranslatef(x, y, 0);	
		draw();
		glPopMatrix();
	}
	
	void OpticalFlowDense::draw(float x, float y, float w, float h) const {
		glPushMatrix();
		glTranslatef(x, y, 0);
		glScalef(w/size.x, h/size.y, 1.0);
		draw();
		glPopMatrix();
	}
	
	void OpticalFlowDense::setROI(int x, int y, int w, int h) {
//		cvSetImageROI(cvImageVel_x, cvRect(x,y, w,h) );
//		cvSetImageROI(cvImageVel_y, cvRect(x,y, w,h) );
        cvSetImageROI(cvFlow,  cvRect(x,y, w,h) );
		roiRect = ofRectangle(x, y, w, h);
		invSize = Vec2f(1.0f/w, 1.0f/h);
	}
	
	void OpticalFlowDense::resetROI() {
//		cvResetImageROI(cvImageVel_x);
//		cvResetImageROI(cvImageVel_y);
        cvResetImageROI(cvFlow);
		roiRect = ofRectangle(0, 0, size.x, size.y);
		invSize = Vec2f(1.0f/size.x, 1.0f/size.y);
	}
	
}