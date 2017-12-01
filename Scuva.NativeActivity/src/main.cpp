// Scuva: main.cpp

// Include config defintions header
#include "config.h"

// Include local headers
#include "log.h"
//#include "VtoK.h"

// Include standard libraries
#include <string>
#include <chrono>
#include <ctime>

// Include android libraries
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"


/*
* Our saved state data.
*/
struct saved_state {
	float angle;
	int32_t x;
	int32_t y;
};

/*
 * Shared state for our app.
 */
struct decoder 
{
	AMediaExtractor* ex;
	AMediaCodec *codec;
	int64_t render_start;
	bool sawInputEOS;
	bool sawOutputEOS;
	bool isPlaying;
	bool render_once;
	int64_t renderstart;
	float speed;
};

struct engine
{
	struct android_app* app;
	decoder decoder;

	ASensorManager* sensorManager;
	const ASensor* accelerometerSensor;
	ASensorEventQueue* sensorEventQueue;

	int animating;
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	struct saved_state state;
};

/*
 * Initialize an EGL context for the current display and Decoder
 */
static int engine_init_display(struct engine* engine) 
{
	// Prepare to decode a video file
	LOGI("Loading video file...");
	std::string path = Log::Path() + std::string("/test.mp4");

	// Determine media file format
	AMediaFormat *media_format = AMediaExtractor_getTrackFormat(engine->app->media_extract, 0);
	const char *s = AMediaFormat_toString(media_format);
	LOGD("Track %d, Format: %s", 0, s);
	const char *mime;
	AMediaFormat_getString(media_format, AMEDIAFORMAT_KEY_MIME, &mime);
	LOGD("Codec: %s", mime);
	AMediaExtractor_selectTrack(engine->app->media_extract, 0);
	engine->decoder.codec = AMediaCodec_createDecoderByType(mime);
	AMediaCodec_configure(engine->decoder.codec, media_format, engine->app->window, NULL, 0);
	engine->decoder.ex = engine->app->media_extract;
	engine->decoder.renderstart = -1;
	engine->decoder.sawInputEOS = false;
	engine->decoder.sawOutputEOS = false;
	engine->decoder.isPlaying = false;
	engine->decoder.render_once = true;
	engine->decoder.speed = 1.0f;

	// Start Media Codec
	AMediaCodec_start(engine->decoder.codec);

	// Initialize OpenGL ES and EGL

	/*
	* Here specify the attributes of the desired configuration.
	* Below, we select an EGLConfig with at least 8 bits per color
	* component compatible with on-screen windows
	*/
	const EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};
	EGLint w, h, format;
	EGLint numConfigs;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	EGLint major;
	EGLint minor;

	eglInitialize(display, &major, &minor);

	/* Here, the application chooses the configuration it desires. In this
	* sample, we have a very simplified selection process, where we pick
	* the first EGLConfig that matches our criteria */
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	
	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	* guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	* As soon as we picked a EGLConfig, we can safely reconfigure the
	* ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

	//ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

	surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);

	LOGD("Creating Context...");

	// EGL_CONTEXT_CLIENT_VERSION = 0x3098;
	const int attrib_list[] = { 0x3098, (int)2, EGL_NONE};
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrib_list);
	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}
	LOGD("Done");

	// Report OpenGL version
	LOGD("OpenGL version is (%s)\n", glGetString(GL_VERSION));

	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	engine->display = display;
	engine->context = context;
	engine->surface = surface;
	engine->width = w;
	engine->height = h;
	engine->state.angle = 0;

	LOGD("Debuggery: Starting...");
	return 0;
}

// A simple time measurmeent function
int64_t systemnanotime() 
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec * 1000000000LL + now.tv_nsec;
}


/*
 * Just the current frame in the display.
 */

static void engine_draw_frame(struct engine* engine) 
{
	if (engine->display == NULL) {
		// No display.
		return;
	}

	// Record start time
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

	// Get most recent CODEC results
	// Set a new input frame
	int bufidx = -1;
	if (!engine->decoder.sawInputEOS)
	{
		size_t bufsize;

		// Retrieve an available input buffer
		bufidx = AMediaCodec_dequeueInputBuffer(engine->decoder.codec, 2000);
		//LOGI("Input buffer idx:  %d", bufidx);
		if (bufidx >= 0)
		{
			auto buf = AMediaCodec_getInputBuffer(engine->decoder.codec, bufidx, &bufsize);
			//LOGD("Input buffer size: %i", bufsize);
			auto sampleSize = AMediaExtractor_readSampleData(engine->app->media_extract, buf, bufsize);
			//LOGD("Input sample size: %i", sampleSize);
			if (sampleSize < 0)
			{
				sampleSize = 0;
				engine->decoder.sawInputEOS = true;
				LOGD("Decoder: input EOS");
			}
			auto presentationTimeUs = AMediaExtractor_getSampleTime(engine->decoder.ex);

			AMediaCodec_queueInputBuffer(
				engine->decoder.codec,
				bufidx, 0, sampleSize, presentationTimeUs,
				engine->decoder.sawInputEOS ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM : 0);
			AMediaExtractor_advance(engine->decoder.ex);
		}
	}

	// Fill output buffer (maybe)
	if (!engine->decoder.sawOutputEOS)
	{
		AMediaCodecBufferInfo info;
		auto status = AMediaCodec_dequeueOutputBuffer(engine->decoder.codec, &info, 0);
		if (status >= 0)
		{
			if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
			{
				LOGI("Decoder: output EOS");
				engine->decoder.sawOutputEOS = true;
			}
			int64_t presentationNano = (int64_t)((float)info.presentationTimeUs * 1000.f * engine->decoder.speed);
			if (engine->decoder.renderstart < 0)
			{
				engine->decoder.renderstart = systemnanotime() - presentationNano;
			}
			int64_t delay = (engine->decoder.renderstart + presentationNano) - systemnanotime();
			if (delay > 0)
			{
				usleep(delay / 1000);
			}
			AMediaCodec_releaseOutputBuffer(engine->decoder.codec, status, info.size != 0);
			if (engine->decoder.render_once)
			{
				engine->decoder.render_once = false;
				return;
			}
		}
		else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
		{
			LOGI("output buffers changed");
		}
		else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
		{
			auto format = AMediaCodec_getOutputFormat(engine->decoder.codec);
			LOGI("format changed to: %s", AMediaFormat_toString(format));
			AMediaFormat_delete(format);
		}
		else if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
			//LOGI("no output buffer right now");
		}
		else {
			LOGI("unexpected info code: %zd", status);
		}
	}

	// DISPLAY

	// Swap buffers
	//eglSwapBuffers(engine->display, engine->surface);

	// Record End time
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

	// Report performance
	long duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	//LOGD("Elapsed time: %d us", duration);

}

/*
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}
	engine->animating = 0;
	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;

	// VtoK Cleanup
	//device.Cleanup();
}

/**
* Process the next input event.
*/
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	struct engine* engine = (struct engine*)app->userData;
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		engine->state.x = AMotionEvent_getX(event, 0);
		engine->state.y = AMotionEvent_getY(event, 0);
		return 1;
	}
	return 0;
}

/**
* Process the next main command.
*/
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	struct engine* engine = (struct engine*)app->userData;
	switch (cmd) {
	case APP_CMD_SAVE_STATE:
		// The system has asked us to save our current state.  Do so.
		engine->app->savedState = malloc(sizeof(struct saved_state));
		*((struct saved_state*)engine->app->savedState) = engine->state;
		engine->app->savedStateSize = sizeof(struct saved_state);
		break;
	case APP_CMD_INIT_WINDOW:
		// The window is being shown, get it ready.
		if (engine->app->window != NULL) {
			engine_init_display(engine);
			engine_draw_frame(engine);
		}
		break;
	case APP_CMD_TERM_WINDOW:
		// The window is being hidden or closed, clean it up.
		engine_term_display(engine);
		break;
	case APP_CMD_GAINED_FOCUS:
		// When our app gains focus, we start monitoring the accelerometer.
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_enableSensor(engine->sensorEventQueue,
				engine->accelerometerSensor);
			// We'd like to get 60 events per second (in us).
			ASensorEventQueue_setEventRate(engine->sensorEventQueue,
				engine->accelerometerSensor, (1000L / 60) * 1000);
		}
		break;
	case APP_CMD_LOST_FOCUS:
		// When our app loses focus, we stop monitoring the accelerometer.
		// This is to avoid consuming battery while not being used.
		if (engine->accelerometerSensor != NULL) {
			ASensorEventQueue_disableSensor(engine->sensorEventQueue,
				engine->accelerometerSensor);
		}
		// Also stop animating.
		engine->animating = 0;
		engine_draw_frame(engine);
		break;
	}
}

/**
* This is the main entry point of a native application that is using
* android_native_app_glue.  It runs in its own thread, with its own
* event loop for receiving input events and doing other things.
*/
void android_main(struct android_app* state) 
{
	LOGI("Starting Android main...");

	// Get external storage path (using JNI)
	std::string base_path = std::string(state->activity->externalDataPath);
	LOGI("base path: %s", base_path.c_str());

	// Start Log
	Log::Initialize(base_path);

	// Initialize app state
	struct engine engine;
	memset(&engine, 0, sizeof(engine));
	state->userData = &engine;
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app = state;

	// Prepare to monitor accelerometer
	engine.sensorManager = ASensorManager_getInstance();
	engine.accelerometerSensor = ASensorManager_getDefaultSensor(engine.sensorManager, ASENSOR_TYPE_ACCELEROMETER);
	engine.sensorEventQueue = ASensorManager_createEventQueue(engine.sensorManager, state->looper, LOOPER_ID_USER, NULL, NULL);
	if (state->savedState != NULL) 
	{
		// We are starting with a previous saved state; restore from it.
		engine.state = *(struct saved_state*)state->savedState;
	}
	engine.animating = 1;

	while (1)
	{
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		// If not animating, we will block forever waiting for events.
		// If animating, we loop until all events are read, then continue
		// to draw the next frame of animation.
		while ((ident = ALooper_pollAll(engine.animating ? 0 : -1, NULL, &events,
			(void**)&source)) >= 0) {

			// Process this event.
			if (source != NULL) {
				source->process(state, source);
			}

			// If a sensor has data, process it now.
			if (ident == LOOPER_ID_USER) {
				if (engine.accelerometerSensor != NULL) {
					ASensorEvent event;
					while (ASensorEventQueue_getEvents(engine.sensorEventQueue, &event, 1) > 0) 
					{
						float rate = event.acceleration.roll / 10.0f;
						if (rate < 0.0f) { rate *= -1.f; }
						if (rate < 0.1f) { rate = 0.1f; }
						engine.decoder.speed = rate + 0.5f;
						LOGI("accelerometer: rate=%f", rate);
					}
				} 
			}
			
			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		if (engine.animating) {
			// Done with events; draw next animation frame.
			
			// Drawing is throttled to the screen update rate, so there
			// is no need to do timing here.
			engine_draw_frame(&engine);
		}
	}
}
