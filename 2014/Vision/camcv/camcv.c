
/////////////////////////////////////////////////////////////
// Many source code lines are copied from RaspiVid.c
// Copyright (c) 2012, Broadcom Europe Ltd
// 
// Lines have been added by Pierre Raufast - mai 2013
// pierre.raufast@gmail.com
// to work with OpenCV 2.3
// visit thinkrpi.wordpress.com
// Enjoy !
// This file display camera in a OpenCv window
//
// For a better world, read Giono's Books
// 
/////////////////////////////////////////////////////////////
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <math.h>
//new
#include <cv.h>
#include <highgui.h>
#include "time.h"

#include "bcm_host.h"
#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

#include "RaspiCamControl.h"
#include "RaspiPreview.h"
#include "RaspiCLI.h"

#include <semaphore.h>

#include <limits.h>

//COM Port stuff
#include "rs232.h"
#define COMPORT		16	// this is '/dev/ttyUSB0' (for arduino) or choose wherever you added '/dev/ttyAMA0' (for raspberry pi UART pins) to the list
#define BAUDRATE	9600	// or whatever baudrate you want
#define RECEIVE_CHARS	1	// or whatever size of buffer you want to receive
#define SEND_CHARS	18	// or whatever size of buffer you want to send
unsigned char	receive_buffer[RECEIVE_CHARS] = {0};
unsigned char	send_buffer[SEND_CHARS];
uint8_t			light_status;
float_t			distAng[2];
uint8_t			cam_mode;

/// Camera number to use - we only have one camera, indexed from 0.
#define CAMERA_NUMBER 0

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

// Video format information
#define VIDEO_FRAME_RATE_NUM 60
#define VIDEO_FRAME_RATE_DEN 1

// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

// Max bitrate we allow for recording
const int MAX_BITRATE = 30000000; // 30Mbits/s


//new 
int nCount=0, colourCheck=0; //DJ added
IplImage *py, *pu, *pv, *pupv, *py_small, *image, *rgbImage, *dstImage, *dst32, *dst32Steer; //*pu_big, *pv_big,
time_t timer_begin,timer_end;
double secondsElapsed;
CvScalar DISPcolour; //= cvScalar(255,0,0,0); //DJ added
CvPoint cSampPt1={135,215};
CvPoint cSampPt2={185,238}; //DJ added
		
int mmal_status_to_int(MMAL_STATUS_T status);

/** Structure containing all state information for the current run 
 * 
 */
typedef struct
{
   long timeout;                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
   int width;                          /// Requested width of image
   int height;                         /// requested height of image
   int bitrate;                        /// Requested bitrate
   int framerate;                      /// Requested frame rate (fps)
   int graymode;			/// capture in gray only (2x faster)
   int immutableInput;      /// Flag to specify whether encoder works in place or creates a new buffer. Result is preview can display either
                                       /// the camera output or the encoder output (with compression artifacts)
   RASPIPREVIEW_PARAMETERS preview_parameters;   /// Preview setup parameters
   RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters

   MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
   MMAL_COMPONENT_T *encoder_component;   /// Pointer to the encoder component
   MMAL_CONNECTION_T *preview_connection; /// Pointer to the connection from camera to preview
   MMAL_CONNECTION_T *encoder_connection; /// Pointer to the connection from camera to encoder

   MMAL_POOL_T *video_pool; /// Pointer to the pool of buffers used by encoder output port
   
} RASPIVID_STATE;

/** Struct used to pass information in encoder port userdata to callback
 *
 */
typedef struct
{
   FILE *file_handle;                   /// File handle to write buffer data to.
   VCOS_SEMAPHORE_T complete_semaphore; /// semaphore which is posted when we reach end of frame (indicates end of capture or fault)
   RASPIVID_STATE *pstate;            /// pointer to our state in case required in callback
} PORT_USERDATA;

//DJ added
// Struct used to get the colour sample in current lighting conditions for thresholding
typedef struct
{
	double uSample;
	double vSample;
} colourSample;

//DJ added
// Struct used for steering offset and angle to be sent over serial connection == 2*sizeof(double)
typedef struct
{
	double angle;
	double offset;
} Steer;

colourSample colourThres;  //DJ added
Steer driveDir; //DJ added

//DJ added
/* Function to sample the Y(UV) colour values in current lighting conditions from the calibration piece of plastic cone
 * 
 */
static colourSample getColourSample(IplImage* uPlane, IplImage* vPlane)
{
	IplImage *subimage;
	CvScalar avgValue;
	colourSample output;
	if (colourCheck)
	{						// if the command argv is not given to do continuous colour sampling for threshold calibration
		output.uSample=100;
		output.vSample=160;
		return output;
	}
	else
	{
		//cvSetImageROI(uPlane, cvRect(135,215,50,25));
		cvSetImageROI(uPlane, cvRect(cSampPt1.x, cSampPt2.y, (cSampPt2.x-cSampPt1.x), (cSampPt2.y-cSampPt1.y) ));
		//subimage = cvCreateImage(cvGetSize(uPlane), uPlane->depth, uPlane->nChannels);
		avgValue = cvAvg(uPlane, NULL);
		output.uSample=avgValue.val[0];  // get the average value of all pixels in the ROI for this uPlane
		cvSetZero(uPlane);		// black out the area of the calibration strip to avoid blob detection problems
		cvResetImageROI(uPlane);
		//cvReleaseImage(&subimage);
		//
		//cvSetImageROI(vPlane, cvRect(135,215,50,25));
		cvSetImageROI(vPlane, cvRect(cSampPt1.x, cSampPt2.y, (cSampPt2.x-cSampPt1.x), (cSampPt2.y-cSampPt1.y) ));
		//subimage = cvCreateImage(cvGetSize(vPlane), vPlane->depth, vPlane->nChannels);
		avgValue = cvAvg(vPlane, NULL);
		output.vSample=avgValue.val[0];  // get the average value of all pixels in the ROI for this vPlane
		cvSetZero(vPlane);		// black out the are of the calibration strip to avoid blob detection problems
		cvResetImageROI(vPlane);
		//cvReleaseImage(&subimage);
		return output;		
	}
}

// DJ added
/* Function amplifies the max value equipotential line in Distance Map
 * 
 */
static void GAKCurve(IplImage* baseDistMap)
{
	int rows;
	double min_val, max_val;
	CvPoint min_pt, max_pt;
	CvScalar temp;
	IplImage* tempImage = cvCloneImage(baseDistMap);  // make a copy
	cvSetZero(baseDistMap);  // zero it out so we can draw only the max equipotential curve
	//fprintf(stdout,"got here");
	for (rows=(tempImage->height-1); rows>=2; rows--)
	{
		cvSetImageROI(tempImage, cvRect(0, rows, tempImage->width, 1));  //set ROI for specific row
		cvSetImageROI(baseDistMap, cvRect(0, rows, baseDistMap->width, 1));
		cvMinMaxLoc(tempImage, &min_val, &max_val, &min_pt, &max_pt, NULL); //with ROI set, find min and max pixel in just that row
		temp.val[0]=cvGetReal1D(tempImage, max_pt.x)+1000;
		//cvSet1D(baseDistMap, max_pt.x, temp);  // amplify the intensity before it is normalized for display later on
		cvSet1D(baseDistMap, max_pt.x, temp);  // amplify the intensity before it is normalized for display later on
		cvResetImageROI(tempImage);
		cvResetImageROI(baseDistMap);
	}
	//fprintf(stdout,"got past for-loop");
	//cvResetImageROI(tempImage);
	//baseDistMap = tempImage;  // make it point to the revised image with only equipotential curve
	cvReleaseImage(&tempImage);
}

//DJ added
/* Function that gets our offset from center and steering angle
 * 
 */
void GAKSteer(IplImage* curveImage, Steer* thing)
{
	CvPoint pt1, pt2, midline={160,200};
	int iOffset, iAngle;
	double max1, max2, dOffset, dAngle;
	DISPcolour = cvScalar(255,0,0,0);
	cvSetImageROI(curveImage, cvRect(0,200,curveImage->width,1));  //set ROI to find max value for pt1
	cvMinMaxLoc(curveImage, NULL, &max1, NULL, &pt1, NULL);
	pt1.y=200; //set manually because ROI is 1-height so y=0
	cvResetImageROI(curveImage);
	cvSetImageROI(curveImage, cvRect(0,100,curveImage->width,1)); //set ROI to find max value for pt2
	cvMinMaxLoc(curveImage, NULL, &max2, NULL, &pt2, NULL);
	pt2.y=100;
	cvResetImageROI(curveImage);
	iOffset = midline.x - pt1.x;
	dOffset = (double)iOffset;
	thing->offset = dOffset;
	if ((pt2.x-pt1.x) == 0)
		dAngle = 90;
	else
		dAngle = atan2((pt1.y-pt2.y),(pt2.x-pt1.x))*(180/3.14159);
	thing->angle = dAngle;
	cvLine(curveImage, pt1, pt2, DISPcolour, 10, 8, 0);
	
}

// default status
static void default_status(RASPIVID_STATE *state)
{
   if (!state)
   {
      vcos_assert(0);
      return;
   }

   // Default everything to zero
   memset(state, 0, sizeof(RASPIVID_STATE));

   // Now set anything non-zero
   state->timeout 			= LONG_MAX;     // 5s delay before take image
   state->width 			= 640;      // use a multiple of 320 (640, 1280)
   state->height 			= 480;		// use a multiple of 240 (480, 960)
   state->bitrate 			= 17000000; // This is a decent default bitrate for 1080p
   state->framerate 			= VIDEO_FRAME_RATE_NUM;
   state->immutableInput 		= 1;
   state->graymode 			= 0;		//gray by default, much faster than color (0)
   
   // Setup preview window defaults
   raspipreview_set_defaults(&state->preview_parameters);

   // Set up the camera_parameters to default
   raspicamcontrol_set_defaults(&state->camera_parameters);
}
  


/**
 *  buffer header callback function for video
 *
 * @param port Pointer to port from which callback originated
 * @param buffer mmal buffer header pointer
 */
static void video_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	MMAL_BUFFER_HEADER_T *new_buffer;
	PORT_USERDATA *pData = (PORT_USERDATA *)port->userdata;

	if (pData)
	{
     
		if (buffer->length)
		{

			mmal_buffer_header_mem_lock(buffer);
	 
	 		//
			// *** PR : OPEN CV Stuff here !
			//
			int w=pData->pstate->width;	// get image size
			int h=pData->pstate->height;
			int h4=h/4;
		
			//memcpy(py->imageData,buffer->data,w*h);	// read Y
			memcpy(pu->imageData,buffer->data+w*h,w*h4); // read U
			memcpy(pv->imageData,buffer->data+w*h+w*h4,w*h4); // read v
			
			if (cam_mode==0) // Cone detection
			{
				cvSmooth(pu,pu,CV_GAUSSIAN,3,0,0,0);
				cvSmooth(pv,pv,CV_GAUSSIAN,3,0,0,0);
				colourThres = getColourSample(pu, pv); // DJ added: samples the YUV value of orange piece in current lighting conditions
				cvThreshold(pu, pu, colourThres.uSample, 255, 1); //Was 110
				cvThreshold(pv, pv, colourThres.vSample, 255, 0); //Was 170

				//cvThreshold(pu, pu, 120, 255, 1); //Was 110
				//cvThreshold(pv, pv, 160, 255, 0); //Was 170
				cvAnd(pv,pv,pupv, NULL);
				cvNot(pupv,pupv);
				cvDistTransform(pupv,dst32,CV_DIST_L2,3,NULL,NULL);
								
				GAKCurve(dst32);  //DJ added
				GAKSteer(dst32, &driveDir); //DJ added
				printf("Offset: %3.2f  and  Angle: %3.2f\n", driveDir.offset, driveDir.angle);

				//cvCornerHarris(pupv,CornerHarris,5,5,0.4);//corner harris
				//cvCanny(pupv,EdgeCanny,50,100,3);//Edge canny

				//cvResize(pupv, pupv_big, CV_INTER_NN);
				//cvResize(pu, pu_big, CV_INTER_NN);
				//cvResize(pv, pv_big, CV_INTER_NN);  //CV_INTER_LINEAR looks better but it's slower
				
				cvNormalize(dst32, dst32Steer, 0.0, 1.0, CV_MINMAX, NULL);
				//GAKCurve(dst32);  //DJ added
				//cvShowImage("norm_camcvWin", dst32Steer);
				DISPcolour=cvScalar(255,0,0,0);
				cvRectangle(dst32, cSampPt1, cSampPt2, DISPcolour, 3, 8, 0); 
				//cvShowImage("camcvPU",pu);  //only for dev purpose //DJ added
				//cvShowImage("camcvPV",pv);
				//cvMerge(py, pu_big, pv_big, NULL, image);
	
				//cvCvtColor(image,dstImage,CV_YCrCb2RGB);	// convert in RGB color space (slow)
				//cvShowImage("camcvWin", dstImage );
				
				light_status = 0; //Not detecting lights
				distAng[0]   = driveDir.offset;		
				distAng[1]   = driveDir.angle;
			}
			else if(cam_mode==1) // Light deteection
			{	
				memcpy(py->imageData,buffer->data,w*h);	// read Y
				cvPyrDown(py, py_small,CV_GAUSSIAN_5x5);
				cvMerge(py_small, pu, pv, NULL, image);
				cvCvtColor(image,rgbImage,CV_YCrCb2RGB);	// convert in RGB color space (slow)
				int meh;
				//Time to experiment with OpenCV and get some images!!!
				cvInRangeS(rgbImage,cvScalar(0,100,0,0),cvScalar(120,255,120,0),pupv);
				if((meh=cvCountNonZero(pupv))>=800)//needs calibration
				{
					light_status=2;
				}
				else
				{
					light_status=0;
				}
				//cvShowImage("camcvWin", pupv); // display only gray channel
				printf("%d\n",meh);
				
				//light_status = something about the lights;   // 0 - none, 1 - red, 2 - green
				distAng[0]   = 0; //Not detecting cones	
				distAng[1]   = 0; //Not detecting cones
			}
		
			cvWaitKey(1);
			nCount++;		// count frames displayed

			if (!(nCount%60)){
				time(&timer_end);
				secondsElapsed = difftime(timer_end,timer_begin);
		
				printf ("%.f seconds for %d frames : FPS = %f\n", secondsElapsed,nCount,(float)((float)(nCount)/secondsElapsed));
			}
			
			send_buffer[1] = light_status;
			uint8_t* bytePtr = (uint8_t*) distAng;
			int byteCnt;
			for (byteCnt = 3; byteCnt<18; byteCnt+=2){
				send_buffer[byteCnt] = *bytePtr;
				bytePtr++;
			}
			
			RS232_SendBuf(COMPORT, send_buffer, SEND_CHARS);
			RS232_PollComport(COMPORT,receive_buffer,RECEIVE_CHARS);
			cam_mode = receive_buffer[0];
		
		 	mmal_buffer_header_mem_unlock(buffer);
		}
		else vcos_log_error("buffer null");
      
	}
	else
	{
		vcos_log_error("Received a encoder buffer callback with no state");
	}

	// release buffer back to the pool
	mmal_buffer_header_release(buffer);

	// and send one back to the port (if still open)
	if (port->is_enabled)
	{
		MMAL_STATUS_T status;
		new_buffer = mmal_queue_get(pData->pstate->video_pool->queue);

		if (new_buffer)
			status = mmal_port_send_buffer(port, new_buffer);

		if (!new_buffer || status != MMAL_SUCCESS)
			vcos_log_error("Unable to return a buffer to the encoder port");
	}
    
}


/**
 * Create the camera component, set up its ports
 *
 * @param state Pointer to state control struct
 *
 * @return 0 if failed, pointer to component if successful
 *
 */
static MMAL_COMPONENT_T *create_camera_component(RASPIVID_STATE *state)
{
	MMAL_COMPONENT_T *camera = 0;
	MMAL_ES_FORMAT_T *format;
	MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
	MMAL_STATUS_T status;
	
	/* Create the component */
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
	
	if (status != MMAL_SUCCESS)
	{
	   vcos_log_error("Failed to create camera component");
	   goto error;
	}
	
	if (!camera->output_num)
	{
	   vcos_log_error("Camera doesn't have output ports");
	   goto error;
	}
	
	video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
	still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];
	
	//  set up the camera configuration
	{
	   MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
	   {
	      { MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
	      .max_stills_w = state->width,
	      .max_stills_h = state->height,
	      .stills_yuv422 = 0,
	      .one_shot_stills = 0,
	      .max_preview_video_w = state->width,
	      .max_preview_video_h = state->height,
	      .num_preview_video_frames = 3,
	      .stills_capture_circular_buffer_height = 0,
	      .fast_preview_resume = 0,
	      .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
	   };
	   mmal_port_parameter_set(camera->control, &cam_config.hdr);
	}
	// Set the encode format on the video  port
	
	format = video_port->format;
	format->encoding_variant = MMAL_ENCODING_I420;
	format->encoding = MMAL_ENCODING_I420;
	format->es->video.width = state->width;
	format->es->video.height = state->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = state->width;
	format->es->video.crop.height = state->height;
	format->es->video.frame_rate.num = state->framerate;
	format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;
	
	status = mmal_port_format_commit(video_port);
	if (status)
	{
	   vcos_log_error("camera video format couldn't be set");
	   goto error;
	}
	
	// PR : plug the callback to the video port 
	status = mmal_port_enable(video_port, video_buffer_callback);
	if (status)
	{
	   vcos_log_error("camera video callback2 error");
	   goto error;
	}

   // Ensure there are enough buffers to avoid dropping frames
   if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
      video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;


   // Set the encode format on the still  port
   format = still_port->format;
   format->encoding = MMAL_ENCODING_OPAQUE;
   format->encoding_variant = MMAL_ENCODING_I420;
   format->es->video.width = state->width;
   format->es->video.height = state->height;
   format->es->video.crop.x = 0;
   format->es->video.crop.y = 0;
   format->es->video.crop.width = state->width;
   format->es->video.crop.height = state->height;
   format->es->video.frame_rate.num = 1;
   format->es->video.frame_rate.den = 1;

   status = mmal_port_format_commit(still_port);
   if (status)
   {
      vcos_log_error("camera still format couldn't be set");
      goto error;
   }

	
	//PR : create pool of message on video port
	MMAL_POOL_T *pool;
	video_port->buffer_size = video_port->buffer_size_recommended;
	video_port->buffer_num = video_port->buffer_num_recommended;
	pool = mmal_port_pool_create(video_port, video_port->buffer_num, video_port->buffer_size);
	if (!pool)
	{
		vcos_log_error("Failed to create buffer header pool for video output port");
	}
	state->video_pool = pool;

	/* Ensure there are enough buffers to avoid dropping frames */
	if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;
	
	/* Enable component */
	status = mmal_component_enable(camera);
	
	if (status)
	{
		vcos_log_error("camera component couldn't be enabled");
		goto error;
	}
	
	//state->camera_parameters.imageEffect = MMAL_PARAM_IMAGEFX_EMBOSS;
	raspicamcontrol_set_all_parameters(camera, &state->camera_parameters);
	
	state->camera_component = camera;
	
	return camera;

error:

   if (camera)
      mmal_component_destroy(camera);

   return 0;
}

/**
 * Destroy the camera component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroy_camera_component(RASPIVID_STATE *state)
{
   if (state->camera_component)
   {
      mmal_component_destroy(state->camera_component);
      state->camera_component = NULL;
   }
}


/**
 * Destroy the encoder component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroy_encoder_component(RASPIVID_STATE *state)
{
   // Get rid of any port buffers first
   if (state->video_pool)
   {
      mmal_port_pool_destroy(state->encoder_component->output[0], state->video_pool);
   }

   if (state->encoder_component)
   {
      mmal_component_destroy(state->encoder_component);
      state->encoder_component = NULL;
   }
}

/**
 * Connect two specific ports together
 *
 * @param output_port Pointer the output port
 * @param input_port Pointer the input port
 * @param Pointer to a mmal connection pointer, reassigned if function successful
 * @return Returns a MMAL_STATUS_T giving result of operation
 *
 */
static MMAL_STATUS_T connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection)
{
   MMAL_STATUS_T status;

   status =  mmal_connection_create(connection, output_port, input_port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);

   if (status == MMAL_SUCCESS)
   {
      status =  mmal_connection_enable(*connection);
      if (status != MMAL_SUCCESS)
         mmal_connection_destroy(*connection);
   }

   return status;
}

/**
 * Checks if specified port is valid and enabled, then disables it
 *
 * @param port  Pointer the port
 *
 */
static void check_disable_port(MMAL_PORT_T *port)
{
   if (port && port->is_enabled)
      mmal_port_disable(port);
}

/**
 * Handler for sigint signals
 *
 * @param signal_number ID of incoming signal.
 *
 */
static void signal_handler(int signal_number)
{
   // Going to abort on all signals
   vcos_log_error("Aborting program\n");

   // TODO : Need to close any open stuff...how?

   exit(255);
}

/**
 * main
 */
int main(int argc, const char **argv)
{
	// Open COM Port
	RS232_OpenComport(COMPORT, BAUDRATE);
	int i;
	for (i = 0; i < 18; i+=2){ 
		send_buffer[i] = i/2;
	}
	cam_mode = 0;
	
	if (argc == 2) //DJ added
	{
		colourCheck=1;  // if any cmd-line arg is given we threshold based on sampling orange plastic in bottom center of image
	}
	// Our main data storage vessel..
	RASPIVID_STATE state;
	
	MMAL_STATUS_T status = -1;
	MMAL_PORT_T *camera_video_port = NULL;
	MMAL_PORT_T *camera_still_port = NULL;
	MMAL_PORT_T *preview_input_port = NULL;
	MMAL_PORT_T *encoder_input_port = NULL;
	MMAL_PORT_T *encoder_output_port = NULL;
	
	bcm_host_init();
	signal(SIGINT, signal_handler);

	// read default status
	default_status(&state);

	// init windows and OpenCV Stuff
	//cvNamedWindow("camcvPU", CV_WINDOW_AUTOSIZE);
	//cvNamedWindow("camcvPV", CV_WINDOW_AUTOSIZE);
	//cvNamedWindow("norm_camcvWin", CV_WINDOW_AUTOSIZE);
	int w=state.width;
	int h=state.height;
	dstImage = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 1); //DJ added
	py = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);		// Y component of YUV I420 frame
	py_small = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 1);
	pu = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 1);	// U component of YUV I420 frame
	pv = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 1);	// V component of YUV I420 frame
	pupv = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 1);
	//pu_big = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);
	//pv_big = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);
	//pupv_big = cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1);	
	image = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 3);	// final picture to display
	rgbImage = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_8U, 3);
	dst32 = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_32F, 1); //DJ added
	dst32Steer = cvCreateImage(cvSize(w/2,h/2), IPL_DEPTH_32F, 1); //DJ added
   
	// create camera
	if (!create_camera_component(&state))
	{
	   vcos_log_error("%s: Failed to create camera component", __func__);
	}
	else if ((status = raspipreview_create(&state.preview_parameters)) != MMAL_SUCCESS)
	{
	   vcos_log_error("%s: Failed to create preview component", __func__);
	   destroy_camera_component(&state);
	}
	else
	{
		PORT_USERDATA callback_data;
		
		camera_video_port   = state.camera_component->output[MMAL_CAMERA_VIDEO_PORT];
		camera_still_port   = state.camera_component->output[MMAL_CAMERA_CAPTURE_PORT];
	   
		VCOS_STATUS_T vcos_status;
		
		callback_data.pstate = &state;
		
		vcos_status = vcos_semaphore_create(&callback_data.complete_semaphore, "RaspiStill-sem", 0);
		vcos_assert(vcos_status == VCOS_SUCCESS);
		
		// assign data to use for callback
		camera_video_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;
        
        	// init timer
  		time(&timer_begin); 

       
       		// start capture
		if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
		{
		   goto error;
		}
		
		// Send all the buffers to the video port
		
		int num = mmal_queue_length(state.video_pool->queue);
		int q;
		for (q=0;q<num;q++)
		{
		   MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state.video_pool->queue);
		
		   if (!buffer)
		   		vcos_log_error("Unable to get a required buffer %d from pool queue", q);
		
			if (mmal_port_send_buffer(camera_video_port, buffer)!= MMAL_SUCCESS)
		    	vcos_log_error("Unable to send a buffer to encoder output port (%d)", q);
		}
		
		
		// Now wait until we need to stop
		vcos_sleep(state.timeout);
  
error:

		mmal_status_to_int(status);
		
		
		// Disable all our ports that are not handled by connections
		check_disable_port(camera_still_port);
		
		if (state.camera_component)
		   mmal_component_disable(state.camera_component);
		
		//destroy_encoder_component(&state);
		raspipreview_destroy(&state.preview_parameters);
		destroy_camera_component(&state);
		
		}
		if (status != 0)
		raspicamcontrol_check_configuration(128);
		
		time(&timer_end);  /* get current time; same as: timer = time(NULL)  */
		cvReleaseImage(&dstImage);  //DJ added
		cvReleaseImage(&pu);
		cvReleaseImage(&pv);
		cvReleaseImage(&py);
		cvReleaseImage(&py_small);
		cvReleaseImage(&pupv);
		cvReleaseImage(&rgbImage);
		cvReleaseImage(&image);
		//cvReleaseImage(&pupv_big);		
		cvReleaseImage(&dst32);  //DJ added
		//cvReleaseImage(&pv_big);
		
		secondsElapsed = difftime(timer_end,timer_begin);
		
		printf ("%.f seconds for %d frames : FPS = %f\n", secondsElapsed,nCount,(float)((float)(nCount)/secondsElapsed));
		
   return 0;
}

