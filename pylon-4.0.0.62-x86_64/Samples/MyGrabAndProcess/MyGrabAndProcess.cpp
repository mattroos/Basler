// MyGrabAndProcess.cpp
/*
   This sample illustrates how to grab and process images using the CInstantCamera class.
   The images are grabbed and processed asynchronously, i.e.,
   while the application is processing a buffer, the acquisition of the next buffer is done
   in parallel.

   The CInstantCamera class uses a pool of buffers to retrieve image data
   from the camera device. Once a buffer is filled and ready,
   the buffer can be retrieved from the camera object for processing. The buffer
   and additional image data are collected in a grab result. The grab result is
   held by a smart pointer after retrieval. The buffer is automatically reused
   when explicitly released or when the smart pointer object is destroyed.
*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbInstantCamera.h>   // MJR
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

//#include "../include/SampleImageCreator.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <time.h>
#include <iostream>
#include <unistd.h>
#include <math.h>
using namespace cv;
using namespace std;



// Namespace for using pylon objects.
using namespace Pylon;

using namespace GenApi; // needed for image read/write? -MJR


// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 5;

clock_t tStart;
clock_t tCount;

int frameCount = 0;
char frameFilename[100];

int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;

    // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
    // is initialized during the lifetime of this object.
    Pylon::PylonAutoInitTerm autoInitTerm;

    // HOW TO SET EXPOSURE TIME?
    // HOW TO SET ANALOG GAIN?
    // HOW TO SET GAMMA?

    try
    {
        // Create an instant camera object with the camera device found first.
        //CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());
        CBaslerUsbInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice()); //MJR

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        //camera.RegisterConfiguration( new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
        camera.Open();
        camera.MaxNumBuffer = 5;

        // This smart pointer will receive the grab result data.
        CGrabResultPtr ptrGrabResult;

        // Print out some camera settings/info
        cout << "===========" << endl;
        cout << "Height Max: " << camera.Height.GetMax() << endl;
        cout << "Height Min: " << camera.Height.GetMin() << endl;
        cout << "Width Max: " << camera.Width.GetMax() << endl;
        cout << "Width Min: " << camera.Width.GetMin() << endl;
        cout << "ExposureTime (us): " << camera.ExposureTime.GetValue() << endl;
        cout << "===========" << endl << endl;


        int64_t increments = (camera.Height.GetMax() - camera.Height.GetMin() + 1) / camera.Height.GetInc();
        // cout << "Increments: " << increments << endl;
        camera.Height.SetValue( camera.Height.GetInc() * (increments / 1));


        tStart = clock();

        while (frameCount < 4)
        {
            // // Set height of image (cuts off bottom portion)
            // camera.Height.SetValue( camera.Height.GetInc() * (increments / (frameCount+1)));

            // Set exposure time in microseconds
            camera.ExposureTime.SetValue( pow(10.0,frameCount+2) );

            // Grab a single image
            if (camera.GrabOne(5000,ptrGrabResult,TimeoutHandling_ThrowException))
            {
                cout << "Got single image." << endl;

                frameCount = frameCount + 1;

                // // Access the image data.
                // cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                // cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                // const uint8_t *pImageBuffer = (uint8_t *) ptrGrabResult->GetBuffer();
                // cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;

                // // Save the image using Pylon API
                // CImagePersistence::Save( ImageFileFormat_Png, "GrabbedImage.png", ptrGrabResult);


                // Copy frame to OpenCV Mat image...
                // suppose your camera is monochrome... get a pointer to pylon image 
                //const pylon::uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
                int frameCols = ptrGrabResult->GetWidth();
                int frameRows = ptrGrabResult->GetHeight();
                // Map the pylon image buffer to a cv::Mat (create a cv::Mat from external buffer)
                
                //theFrame = cv::Mat(cv::Size(frameCols, frameRows), CV_8UC1, (void*)pImageBuffer, cv::Mat::AUTO_STEP);
                //Mat theFrame = cv::Mat(cv::Size(frameCols, frameRows), CV_8UC1, (void*)pImageBuffer);
                Mat theFrame = cv::Mat(frameRows, frameCols, CV_8UC1);
                memcpy(theFrame.ptr(),(uint8_t *)ptrGrabResult->GetBuffer(),frameCols*frameRows);


                // Save openCV Mat image
                char str[15];
                tCount = clock() - tStart;
                sprintf(str, "%f seconds", ((float)tCount)/CLOCKS_PER_SEC);
                printf("%ld: %f seconds\n", tCount, ((float)tCount)/CLOCKS_PER_SEC);

                putText(theFrame,str,cvPoint(30,100),FONT_HERSHEY_SIMPLEX,2,cvScalar(200,200,250),3,CV_AA);
                sprintf(frameFilename,"GrabbedImageCV_%.3d.png",frameCount);
                imwrite(frameFilename,theFrame);
            }
            else
            {
                cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }

        camera.Close();

        // // cout << "Height Max: " << camera.Height.GetMax() << endl;
        // // cout << "Height Max: " << camera.Height.GetMin() << endl;
        // // int64_t increments = (camera.Height.GetMax() - camera.Height.GetMin()) / camera.Height.GetInc();
        // // cout << increments << endl;
        // // camera.Height.SetValue( camera.Height.GetInc() * (increments / 4));


        // // The parameter MaxNumBuffer can be used to control the count of buffers
        // // allocated for grabbing. The default value of this parameter is 10.
        // camera.MaxNumBuffer = 5;

        // // Start the grabbing of c_countOfImagesToGrab images.
        // // The camera device is parameterized with a default configuration which
        // // sets up free-running continuous acquisition.
        // camera.StartGrabbing( c_countOfImagesToGrab);

        // // This smart pointer will receive the grab result data.
        // CGrabResultPtr ptrGrabResult;

        // // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
        // // when c_countOfImagesToGrab images have been retrieved.
        // tStart = clock();
        // while (camera.IsGrabbing())
        // {
        //     // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
        //     camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException);

        //     // Image grabbed successfully?
        //     if (ptrGrabResult->GrabSucceeded())
        //     {
        //         frameCount = frameCount + 1;

        //         // Access the image data.
        //         cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
        //         cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
        //         const uint8_t *pImageBuffer = (uint8_t *) ptrGrabResult->GetBuffer();
        //         cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;

        //         // Save the image using Pylon API
        //         CImagePersistence::Save( ImageFileFormat_Png, "GrabbedImage.png", ptrGrabResult);

        //         // suppose your camera is monochrome... get a pointer to pylon image 
        //         //const pylon::uint8_t *pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
        //         int frameCols = ptrGrabResult->GetWidth();
        //         int frameRows = ptrGrabResult->GetHeight();
        //         // Map the pylon image buffer to a cv::Mat (create a cv::Mat from external buffer)
                
        //         //theFrame = cv::Mat(cv::Size(frameCols, frameRows), CV_8UC1, (void*)pImageBuffer, cv::Mat::AUTO_STEP);
        //         //Mat theFrame = cv::Mat(cv::Size(frameCols, frameRows), CV_8UC1, (void*)pImageBuffer);
        //         Mat theFrame = cv::Mat(frameRows, frameCols, CV_8UC1);
        //         memcpy(theFrame.ptr(),(uint8_t *)ptrGrabResult->GetBuffer(),frameCols*frameRows);




        //         // Save openCV Mat frame
        //         char str[15];
        //         tCount = clock() - tStart;
        //         sprintf(str, "%f seconds", ((float)tCount)/CLOCKS_PER_SEC);
        //         printf("%ld: %f seconds\n", tCount, ((float)tCount)/CLOCKS_PER_SEC);
        //         sleep(1);
        //         tCount = clock() - tStart;
        //         sprintf(str, "%f seconds", ((float)tCount)/CLOCKS_PER_SEC);
        //         printf("%ld: %f seconds\n", tCount, ((float)tCount)/CLOCKS_PER_SEC);

        //         putText(theFrame,str,cvPoint(30,100),FONT_HERSHEY_SIMPLEX,2,cvScalar(200,200,250),3,CV_AA);
        //         sprintf(frameFilename,"GrabbedImageCV_%.3d.png",frameCount);
        //         imwrite(frameFilename,theFrame);
        //         // imwrite("GrabbedImageCV.png",theFrame);



        //         // keep a copy of it
        //         //cv::Mat myFrame;
        //         //theFrame.copyTo(myFrame); // myFrame life cycle is now under your control

        //         // // Convert of opencv format and display
        //         // CImageFormatConverter fc;
        //         // //fc.OutputPixelFormat = PixelType_BGR8packed;
        //         // fc.OutputPixelFormat = PixelType_Mono8;
        //         // CPylonImage image;
        //         // fc.Convert(image, ptrGrabResult);

        //         // Mat cv_img = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1,(uint8_t*)image.GetBuffer());
        //         // //imshow(src_window,cv_img);  // display the image in OpenCV image window
        //         // //waitKey(1);

        //     }
        //     else
        //     {
        //         cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
        //     }
        // }
    }
    catch (GenICam::GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Comment the following two lines to disable waiting on exit.
    // cerr << endl << "Press Enter to exit." << endl;
    // while( cin.get() != '\n');

    return exitCode;
}
