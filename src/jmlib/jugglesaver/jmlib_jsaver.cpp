/*
 * JMLib - Portable JuggleMaster Library
 * Version 2.1
 * Copyright (c) Per Johan Groland 2000-2008, Gary Briggs 2003
 *
 * Based on JuggleMaster Version 1.60
 * Copyright (c) 1995-1996 Ken Matsuoka
 *
 * JuggleSaver support based on Juggler3D
 * Copyright (c) 2005-2008 Brian Apps <brian@jugglesaver.co.uk>
 *
 * You may redistribute and/or modify JMLib under the terms of the
 * Modified BSD License as published in various places online or in the
 * COPYING.jmlib file in the package you downloaded.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the
 * Modified BSD License for more details.
 */ 

#ifdef JUGGLESAVER_SUPPORT

// fixme: port dofps

#include "jmlib_jsaver.h"
#include <time.h>
#include "gltrackball.h"

JML_CHAR *JuggleSaver::possible_styles[] = {
	"JuggleSaver",
};

// Constructor / Destructor
JuggleSaver::JuggleSaver()  : JuggleSpeed(3.0f), TranslateSpeed(0.0f), SpinSpeed(20.0f),
  initialized(false), is_juggling(false), pattern(NULL), siteswap(NULL), pattname(NULL),
  width_(480), height_(400), trackball(NULL), spin(TRUE), SavedSpinSpeed(20.0f),
  SavedTranslateSpeed(0.0f) {
  // NOTE:
  // initialize cannot be called from the constructor, because it requires an OpenGL
  // context. It must be called manually after creating the OpenGL context.
  
  CurrentFrameRate = 1.0f;
  FramesSinceSync = 0;
  LastSyncTime = 0;
}

JuggleSaver::~JuggleSaver() {
  shutdown();
  if (pattname != NULL) { delete pattname; }
  if (pattern != NULL) { delete pattern; }
  if (siteswap != NULL) { delete siteswap; }
}

void JuggleSaver::initialize() {
  if (initialized) return;
  
  setWindowSize(width_, height_);
  InitGLSettings(&state, FALSE);
	state.trackball = gltrackball_init();
	trackball = state.trackball;
  applyPattern();
  ResizeGL(&state, width_, height_);
  initialized = true;
}

void JuggleSaver::reInitialize() {
	trackball_state* trackball = state.trackball;

  setWindowSize(width_, height_);
  InitGLSettings(&state, FALSE);
	state.trackball = trackball;
  applyPattern();
  ResizeGL(&state, width_, height_);
  initialized = true;
}

void JuggleSaver::shutdown() {
  free(trackball);
  trackball = NULL;
  state.trackball = NULL;
}

JML_BOOL JuggleSaver::applyPattern() {
  if (siteswap == NULL) setPatternDefault();
  SetPattern(&state, siteswap);
	return 1;
}
  
JML_BOOL JuggleSaver::setPattern(JML_CHAR* name, JML_CHAR* site, JML_FLOAT hr, JML_FLOAT dr) {
  if (name == NULL || site == NULL) return false;

  if (!JSValidator::validateJSPattern(site)) {
    error("Invalid pattern");
    return false;
  }

  if (pattname != NULL) { delete[] pattname; }
  pattname = new JML_CHAR[strlen(name)+1];
  strcpy(pattname, name);

  if (pattern != NULL) { delete[] pattern; }
  pattern = new JML_CHAR[strlen(site)+1];
  strcpy(pattern, site);

	// get the site
	char* s = GetCurrentSite();
  if (siteswap != NULL) { delete[] siteswap; }
  siteswap = new JML_CHAR[strlen(s)+1];
  strcpy(siteswap, s);
	free(s);
  
	if (initialized) {
		resetCamera();
		SetPattern(&state, site);
	}

	return true;
}

void JuggleSaver::setPatternDefault(void) {
  setPattern("3 Cascade", "3");
}

JML_INT32 JuggleSaver::numBalls(void) {
  PATTERN_INFO* pPattern = state.pPattern;
  if (pPattern) return pPattern->Objects;
  return 0;
}

void JuggleSaver::startJuggle(void) {
  is_juggling = true;
}

void JuggleSaver::stopJuggle(void) {
  is_juggling = false;
}

void JuggleSaver::togglePause(void) {
  is_juggling = !is_juggling;
}

void JuggleSaver::setPause(JML_BOOL pauseOn) {
  is_juggling = !pauseOn;
}

JML_INT32 JuggleSaver::getStatus(void) {
  if (!initialized) return ST_NONE;
  if (is_juggling) return ST_JUGGLE;
  return ST_PAUSE;
}

void JuggleSaver::render() {
  DrawGLScene(&state);
}

JML_INT32 JuggleSaver::doJuggle(void) {
  if (!initialized) return 0;

  if (!is_juggling) {
    FramesSinceSync = 0;
    return 0;
  }

  /* While drawing, keep track of the rendering speed so we can adjust the
  * animation speed so things appear consistent.  The basis of the this
  * code comes from the frame rate counter (fps.c) but has been modified
  * so that it reports the initial frame rate earlier (after 0.02 secs
  * instead of 1 sec).
  */
  if (FramesSinceSync >=  1 * (unsigned int) CurrentFrameRate) {
    struct timeval tvnow;
    unsigned now;
            
    # ifdef GETTIMEOFDAY_TWO_ARGS
      struct timezone tzp;
      gettimeofday(&tvnow, &tzp);
    # else
            gettimeofday(&tvnow);
    # endif
        
    now = (unsigned) (tvnow.tv_sec * 1000000 + tvnow.tv_usec);

    if (FramesSinceSync == 0) {
      LastSyncTime = now;
    }
    else {
      unsigned Delta = now - LastSyncTime;
      if (Delta > 20000) {
        LastSyncTime = now;
        CurrentFrameRate = (FramesSinceSync * 1.0e6f) / Delta;
        FramesSinceSync = 0;
      }
    }
  }
    
  FramesSinceSync++;
    
  //if (CurrentFrameRate > 0.001f /*1.0e-6f*/) {
  if (CurrentFrameRate > 1.0e-6f) {
    state.Time += JuggleSpeed / CurrentFrameRate;
		state.SpinAngle += SpinSpeed / CurrentFrameRate;
		state.TranslateAngle += TranslateSpeed / CurrentFrameRate;
  }
    
	return 1;
}

JML_BOOL JuggleSaver::setWindowSize(JML_INT32 width, JML_INT32 height) {
  width_ = width;
  height_ = height;
  if (initialized) ResizeGL(&state, width_, height_);
  FramesSinceSync = 0;
  return TRUE;
}

JML_INT32 JuggleSaver::getImageWidth() {
  return width_;
}

JML_INT32 JuggleSaver::getImageHeight() {
  return height_;
}

void JuggleSaver::speedUp(void) {
  if (JuggleSpeed <= 10.0f) JuggleSpeed++;
}

void JuggleSaver::speedDown(void) {
  if (JuggleSpeed >= 0.1f) JuggleSpeed--;
}

void JuggleSaver::speedReset(void) {
  JuggleSpeed = 2.2f;
}

void JuggleSaver::setSpeed(float s) {
  if (s < 0.1) s= 0.1f;
  if (s > 10.0) s = 10.0f;
  JuggleSpeed = s;
}

float JuggleSaver::speed(void) {
  return JuggleSpeed;
}

// Returns dummy valiue
// BallRad is used internally for ball radius. This is a float value
JML_INT32 JuggleSaver::getBallRadius(void) {
  return 1;
}

JML_CHAR **JuggleSaver::getStyles(void) {
  return possible_styles;
}

JML_INT32 JuggleSaver::numStyles(void) {
  return (int)(sizeof(possible_styles)/sizeof(possible_styles[0]));

}

JML_BOOL JuggleSaver::isValidPattern(const char* patt) {
	return JSValidator::validateJSPattern((char*)patt);
}

void JuggleSaver::trackballStart(JML_INT32 x, JML_INT32 y) {
	gltrackball_start(trackball, x, y, width_, height_);
}

void JuggleSaver::trackballTrack(JML_INT32 x, JML_INT32 y) {
	gltrackball_track(trackball, x, y, width_, height_);
}

void JuggleSaver::trackballMousewheel(JML_INT32 percent, JML_BOOL horizontal) {
	gltrackball_mousewheel(trackball, 4, percent, horizontal);
}

void JuggleSaver::resetCamera() {
	gltrackball_reset(trackball);
	ResetZoom(&state);
  SpinSpeed = SavedSpinSpeed;
  TranslateSpeed = SavedTranslateSpeed;
}

void JuggleSaver::zoom(float zoom) {
	Zoom(&state, zoom);
}

void JuggleSaver::move(float deltaX, float deltaY) {
	MoveCamera(&state, deltaX, deltaY);
}

void JuggleSaver::toggleAutoRotate() {
  spin = !spin;
  setAutoRotate(spin);
}

void JuggleSaver::setAutoRotate(JML_BOOL on) {
  spin = on;

  if (on) {
    SpinSpeed = SavedSpinSpeed;
    TranslateSpeed = SavedTranslateSpeed;
  }
  else {
    SpinSpeed = 0.0f;
    TranslateSpeed = 0.0f;
  }
}

void JuggleSaver::setAutoRotate(JML_BOOL on, JML_FLOAT spinSpeed, JML_FLOAT translateSpeed) {
  spin = on;

  if (spinSpeed < 0) spinSpeed = 0.0f;
  if (translateSpeed < 0) translateSpeed = 0.0f;

  SavedSpinSpeed = spinSpeed;
  SavedTranslateSpeed = translateSpeed;

  if (on) {
    SpinSpeed = SavedSpinSpeed;
    TranslateSpeed = SavedTranslateSpeed;
  }
  else {
    SpinSpeed = 0.0f;
    TranslateSpeed = 0.0f;
  }
}

#endif // JUGGLESAVER_SUPPORT
