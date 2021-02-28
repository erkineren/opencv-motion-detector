#include "opencv2/opencv.hpp"
#include <sys/timeb.h>
#include <sstream>
#include <string>
#include <iostream>

using namespace cv;
using namespace std;

// local save path for captured images and videos
String const SAVE_FILE_PATH = "/home/erkin/hareket_algilama/";

// remote server address for uploading images and videos
String const UPLOAD_SERVER = "http://192.168.1.2/upload/upload.php";

// options for upload server or not
bool const OPT_UPLOAD_SERVER = true;

// Bool value while saving image => false: full screen camera frame, true: ROI area of frame
bool const SAVE_IMAGE_TO_FILE_AS_ROI_SIZE = false;

// Bool value while saving image to show ROI area with rectangle or not.
// Use this options only if SAVE_IMAGE_TO_FILE_AS_ROI_SIZE is false
bool const SAVE_IMAGE_TO_FILE_WITH_ROI_RECT = true;

// Bool value to show whether ROI rectangle area or not
bool const SHOW_ROI_RECTANGLE = true;

//double const THRESHOLD_OF_MOTION_PARAM = 0.011;
//double const FIRST_FRAME_BLACK_PIXEL_RATE_PARAM = 0.0015;

// multiply this value with first capture black pixel rate. this is threshold for motion. if black pixel rate bigger then the value then movement occured
double const MOTION_THRESHOLD_FACTOR = 8.8;

// when motion is detected, it will ignored how many times
double const MAX_IGNORE_PARASITE_CAPTURE = 2;

// output recorded video will ended after 60 loop if no motion detected
double const MAX_IGNORE_NO_MOTION_COUNT = 60;

// recorded video frame per second
int const OUTPUT_VIDEO_FPS = 9;

double thresholdOfMotion;
double firstFrameBlackPixelRate;
int ignoredParasiteCapture = 0;
int ignoredNoMotionCount = 0;
String outputVideoFilename;

Mat originalFrame, originalFrameWithROIRect; // non-filtered camera frame image
Mat a, b, c; // images for motion detection filter operation.
VideoCapture cap(0); // open the default camera
Rect2d roi;

bool showCrosshair = false;
bool fromCenter = false;

String windowMotionDetect = "Motion Detection";
String windowOriginalData = "Original Stream From Camera";
String windowThreshold = "Threshold Data";

Mat diffres(Mat t0, Mat t1, Mat t2) {
	Mat d1, d2, output;

	// difference between t2 and t1
	absdiff(t2, t1, d1);

	// difference between t1 and t0
	absdiff(t1, t0, d2);

	bitwise_and(d1, d2, output);

	int morph_elem = 0;
	int morph_size = 0;
	Mat element = getStructuringElement(morph_elem,
			Size(2 * morph_size + 1, 2 * morph_size + 1),
			Point(morph_size, morph_size));

	// opening - açýným
	morphologyEx(output, output, CV_MOP_OPEN, element);

	// closing - kapaným
	morphologyEx(output, output, CV_MOP_CLOSE, element);
	threshold(output, output, 10, 255, CV_THRESH_BINARY_INV);

	return output;
}

double getBlackPixelRate(Mat img) {
	// cout << img.rows << " x " << img.cols << endl;
	int blackPoints = 0;

	for (int i = 0; i < img.rows; i++)
		for (int j = 0; j < img.cols; j++)
			if (img.at<uchar> (j, i) == 0) // if pixel is black (value = 0)
				blackPoints++;

	double oran = blackPoints / ((img.rows * img.cols) * 1.0);
	//cout << "Black : " << blackPoints << " / " << img.rows * img.cols << " = "
	//	<< oran << endl;

	return oran;
}

// return current date and time
// this function is used for filename while saving image and video files
String Now() {
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%d-%m-%Y_%H-%M-%S", timeinfo);
	String str(buffer);

	return str;
}

// return current time in milliseconds
// this function is used for filename appendix while saving image and video files
String getMilliCount() {
	timeb tb;
	ftime(&tb);
	int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;

	ostringstream ss;
	ss << nCount;

	return ss.str();
}

// this function is used for selecting region of interest in window
void select_roi() {
	roi = selectROI(originalFrame, showCrosshair, fromCenter);
	cap.read(a);
	cvtColor(a(roi), a, COLOR_RGB2GRAY);
	cap.read(b);
	cvtColor(b(roi), b, COLOR_RGB2GRAY);
	cap.read(c);
	cvtColor(c(roi), c, COLOR_RGB2GRAY);
}

// this function sets motion threshold according to first captured frame from video camera
void set_motion_threshold() {
	firstFrameBlackPixelRate = getBlackPixelRate(diffres(a, b, c));

	//thresholdOfMotion = (firstFrameBlackPixelRate * THRESHOLD_OF_MOTION_PARAM)
	//		/ FIRST_FRAME_BLACK_PIXEL_RATE_PARAM;

	thresholdOfMotion = firstFrameBlackPixelRate * MOTION_THRESHOLD_FACTOR;
	cout << "First Frame Black Pixel Rate: " << firstFrameBlackPixelRate;
	cout << "\nThreshold of Motion: " << thresholdOfMotion << endl;
}

// upload specified file to remote server
void uploadFile(String filename) {
	system(
			("curl -F 'fileToUpload=@" + filename + "' " + UPLOAD_SERVER + " &").c_str());
}

int main(int, char**) {

	cout << "** PATIENT'S MOTION DETECTION PROGRAM **" << endl;
	cout << "OpenCv v" << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << endl;

	if (!cap.isOpened()) { // check if camera opened
		cout << "!! Camera Device Could NOT be opened." << endl;
		return 0;
	}

	//cap.set(CV_CAP_PROP_POS_MSEC, 300); //start the video at 300ms

	double fps = cap.get(CV_CAP_PROP_FPS); //get the frames per seconds of the video
	cout << ":) Camera Device opened successfully with " << "FPS is " << fps
			<< endl;

	//create a windows
	namedWindow(windowMotionDetect, CV_WINDOW_AUTOSIZE);
	namedWindow(windowOriginalData, CV_WINDOW_AUTOSIZE);

	cap.read(originalFrame);

	// Select ROI
	select_roi();

	// set motion threshold
	set_motion_threshold();

	VideoWriter outputVideo; // for saving video
	int ex = static_cast<int> (cap.get(CAP_PROP_FOURCC));
	Size S = Size((int) cap.get(CAP_PROP_FRAME_WIDTH), // Acquire input size
			(int) cap.get(CAP_PROP_FRAME_HEIGHT));

	outputVideoFilename = SAVE_FILE_PATH + "video_" + Now() + ".avi";

	outputVideo.open(outputVideoFilename, ex, OUTPUT_VIDEO_FPS, S, true);

	if (!outputVideo.isOpened()) {
		cout << "Could not open the output video for write" << endl;
		return -1;
	}

	bool isMoving = false;
	bool loop = true;
	while (loop) {
		Mat motionDetection = diffres(a, b, c);
		imshow(windowMotionDetect, motionDetection);

		loop = cap.read(originalFrame);

		if (SHOW_ROI_RECTANGLE) {

			Rect rct(roi.x, roi.y, roi.width, roi.height);
			originalFrameWithROIRect = originalFrame.clone();
			rectangle(originalFrameWithROIRect, rct, CV_RGB(0, 255, 0), 2);

			imshow(windowOriginalData, originalFrameWithROIRect);

		} else {

			imshow(windowOriginalData, originalFrame);

		}

		a = b;
		b = c;
		cap.read(c);
		cvtColor(c(roi), c, COLOR_RGB2GRAY);

		double pixRate = getBlackPixelRate(motionDetection);
		if (pixRate > thresholdOfMotion) { // if has motion
			ignoredParasiteCapture++;
			if (ignoredParasiteCapture >= MAX_IGNORE_PARASITE_CAPTURE) { // ignore some exceeding

				ignoredNoMotionCount = 0;
				isMoving = true;

				ignoredParasiteCapture = 0;
				cout << Now() << "_" << getMilliCount()
						<< ": *** MOVEMENT *** pixRate: " << pixRate
						<< " threshold: " << thresholdOfMotion << endl;

				//save frame to file
				String filename = SAVE_FILE_PATH + "image_" + Now() + "_"
						+ getMilliCount() + ".jpg";
				if (SAVE_IMAGE_TO_FILE_AS_ROI_SIZE) {
					imwrite(filename, originalFrame(roi));
				} else {
					if (SAVE_IMAGE_TO_FILE_WITH_ROI_RECT)
						imwrite(filename, originalFrameWithROIRect);
					else
						imwrite(filename, originalFrame);
				}

				if (OPT_UPLOAD_SERVER)
					uploadFile(filename);
			}
		} else { // if no motion
			if (isMoving) {
				ignoredNoMotionCount++;
				if (ignoredNoMotionCount >= MAX_IGNORE_NO_MOTION_COUNT) {
					ignoredNoMotionCount = 0;
					isMoving = false;
					outputVideo.release();

					cout << "--> VIDEO IS SAVED." << endl;

					if (OPT_UPLOAD_SERVER)
						uploadFile(outputVideoFilename);

					outputVideoFilename = SAVE_FILE_PATH + "video_" + Now()
							+ ".avi";

					outputVideo.open(outputVideoFilename, ex, OUTPUT_VIDEO_FPS,
							S, true);
				}
			}

		}

		if (isMoving) {
			// save frame to video
			outputVideo.write(originalFrame);
		}

		int key = waitKey(5);//wait for key

		switch (key) {
		case 27: //ESC
			cout << "ESC key is pressed by user !" << endl;
			loop = false;
			break;

		case 114: { // r
			select_roi();
			set_motion_threshold();
		}
			break;

		default:
			if (key != -1) {
				char pressed = key;
				cout << "No command assigned for pressed key, " << pressed
						<< endl;
			}
			break;

		}

	}

	return 0;
}
