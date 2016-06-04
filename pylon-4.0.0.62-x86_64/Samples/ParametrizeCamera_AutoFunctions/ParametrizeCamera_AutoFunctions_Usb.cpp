// ParametrizeCamera_AutoFunctions_Usb.cpp
/*
    This sample illustrates how to use the Auto Functions feature of Basler USB cameras.
    
    Features, like 'Gain', are named according to the Standard Feature Naming Convention (SFNC).
    The SFNC defines a common set of features, their behavior, and the related parameter names.
    This ensures the interoperability of cameras from different camera vendors. Cameras compliant
    with the USB 3 Vision standard are based on the SFNC version 2.0.
    Basler GigE and Firewire cameras are based on previous SFNC versions.
    Accordingly, the behavior of these cameras and some parameters names will be different.
    That's why this sample is different from the sample for Firewire and GigE cameras in
    ParametrizeCamera_AutoFunctions.cpp
*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Settings to use  Basler USB cameras.
#include <pylon/usb/BaslerUsbInstantCamera.h>
typedef Pylon::CBaslerUsbInstantCamera Camera_t;
using namespace Basler_UsbCameraParams;

// The camera specific grab result smart pointer.
typedef Camera_t::GrabResultPtr_t GrabResultPtr_t;

bool IsColorCamera(Camera_t& camera);
void AutoGainOnce(Camera_t& camera);
void AutoGainContinuous(Camera_t& camera);
void AutoExposureOnce(Camera_t& camera);
void AutoExposureContinuous(Camera_t& camera);
void AutoWhiteBalance(Camera_t& camera);

int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;

    // Automagically call PylonInitialize and PylonTerminate to ensure that the
    // pylon runtime system is initialized during the lifetime of this object.
    Pylon::PylonAutoInitTerm autoInitTerm;

    try
    {
        // Only look for cameras supported by Camera_t. 
        CDeviceInfo info;
        info.SetDeviceClass( Camera_t::DeviceClass());

        // Create an instant camera object with the first found camera device that matches the specified device class.
        Camera_t camera( CTlFactory::GetInstance().CreateFirstDevice( info));

        // Print the name of the used camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;
        
        // Register the standard event handler for configuring single frame acquisition.
        // This overrides the default configuration as all event handlers are removed by setting the registration mode to RegistrationMode_ReplaceAll.
        // Please note that the camera device auto functions do not require grabbing by single frame acquisition.
        // All available acquisition modes can be used.
        camera.RegisterConfiguration( new CAcquireSingleFrameConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);

        // Open the camera.
        camera.Open();

        // Turn test image off.
        camera.TestImageSelector = TestImageSelector_Off;

        // Only area scan cameras support auto functions.
        if (camera.DeviceScanType.GetValue() == DeviceScanType_Areascan)
        {
            // All area scan cameras support luminance control.
            
            // Carry out luminance control by using the "once" gain auto function.
            // For demonstration purposes only, set the gain to an initial value.
            camera.Gain.SetValue( camera.Gain.GetMax());
            AutoGainOnce(camera);
            cerr << endl << "Press Enter to continue." << endl;
            while( cin.get() != '\n');
            

            // Carry out luminance control by using the "continuous" gain auto function.
            // For demonstration purposes only, set the gain to an initial value.
            camera.Gain.SetValue( camera.Gain.GetMax());
            AutoGainContinuous(camera);
            cerr << endl << "Press Enter to continue." << endl;
            while( cin.get() != '\n');


            // For demonstration purposes only, set the exposure time to an initial value.
            camera.ExposureTime.SetValue( camera.ExposureTime.GetMin());

            // Carry out luminance control by using the "once" exposure auto function.
            AutoExposureOnce(camera);
            cerr << endl << "Press Enter to continue." << endl;
            while( cin.get() != '\n');


            // For demonstration purposes only, set the exposure time to an initial value.     
            camera.ExposureTime.SetValue( camera.ExposureTime.GetMin());

            // Carry out luminance control by using the "continuous" exposure auto function. 
            AutoExposureContinuous(camera);

            // Only color cameras support the balance white auto function.
            if (IsColorCamera(camera))
            {
                cerr << endl << "Press Enter to continue." << endl;
                while( cin.get() != '\n');

                // For demonstration purposes only, set the initial balance ratio values:
                camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
                camera.BalanceRatio.SetValue(3.14);
                camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Green);
                camera.BalanceRatio.SetValue(0.5);
                camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Blue);
                camera.BalanceRatio.SetValue(0.125);
                
                // Carry out white balance using the balance white auto function.
                AutoWhiteBalance(camera);
            }
        }
        else
        {
            cerr << "Only area scan cameras support auto functions." << endl;
        }

        // Close camera.
        camera.Close();

    }
    catch (GenICam::GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press Enter to exit." << endl;
    while( cin.get() != '\n');

    return exitCode;
}


void AutoGainOnce(Camera_t& camera)
{
    // Check whether the gain auto function is available.
    if ( !IsWritable( camera.GainAuto))
    {
        cout << "The camera does not support Gain Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed image area of interest (Image AOI).
    if (IsWritable(camera.OffsetX))
    {
        camera.OffsetX.SetValue(camera.OffsetX.GetMin());
    }
    if (IsWritable(camera.OffsetY))
    {
        camera.OffsetY.SetValue(camera.OffsetY.GetMin());
    }
    camera.Width.SetValue(camera.Width.GetMax());
    camera.Height.SetValue(camera.Height.GetMax());

    // Set the Auto Function AOI for luminance statistics.
    // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
    // luminance statistics.
    camera.AutoFunctionAOISelector.SetValue(AutoFunctionAOISelector_AOI1);
    camera.AutoFunctionAOIOffsetX.SetValue(0);
    camera.AutoFunctionAOIOffsetY.SetValue(0);
    camera.AutoFunctionAOIWidth.SetValue(camera.Width.GetMax());
    camera.AutoFunctionAOIHeight.SetValue(camera.Height.GetMax());

    // Set the target value for luminance control.
    // A value of 0.3 means that the target brightness is 30 % of the maximum brightness of the raw pixel value read out from the sensor.
    // A value of 0.4 means 40 % and so forth.
    camera.AutoTargetBrightness.SetValue(0.3);

    // We are going to try GainAuto = Once.

    cout << "Trying 'GainAuto = Once'." << endl;
    cout << "Initial Gain = " << camera.Gain.GetValue() << endl;

    // Set the gain ranges for luminance control.
    camera.AutoGainLowerLimit.SetValue(camera.Gain.GetMin());
    camera.AutoGainUpperLimit.SetValue(camera.Gain.GetMax());

    camera.GainAuto.SetValue(GainAuto_Once);

    // When the "once" mode of operation is selected, 
    // the parameter values are automatically adjusted until the related image property
    // reaches the target value. After the automatic parameter value adjustment is complete, the auto
    // function will automatically be set to "off" and the new parameter value will be applied to the
    // subsequently grabbed images.
    
    int n = 0;
    while (camera.GainAuto.GetValue() != GainAuto_Off)
    {
        GrabResultPtr_t ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult);
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage(1, ptrGrabResult);
#endif
        ++n;
        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep(100);

        //Make sure the loop is exited.
        if (n > 100)
        {
            throw RUNTIME_EXCEPTION( "The adjustment of auto gain did not finish.");
        }
    }

    cout << "GainAuto went back to 'Off' after " << n << " frames." << endl;
    cout << "Final Gain = " << camera.Gain.GetValue() << endl << endl;
}


void AutoGainContinuous(Camera_t& camera)
{
    // Check whether the Gain Auto feature is available.
    if ( !IsWritable( camera.GainAuto))
    {
        cout << "The camera does not support Gain Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed image area of interest (Image AOI).
    if (IsWritable(camera.OffsetX))
    {
        camera.OffsetX.SetValue(camera.OffsetX.GetMin());
    }
    if (IsWritable(camera.OffsetY))
    {
        camera.OffsetY.SetValue(camera.OffsetY.GetMin());
    }
    camera.Width.SetValue(camera.Width.GetMax());
    camera.Height.SetValue(camera.Height.GetMax());

    // Set the Auto Function AOI for luminance statistics.
    // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
    // luminance statistics.
    camera.AutoFunctionAOISelector.SetValue(AutoFunctionAOISelector_AOI1);
    camera.AutoFunctionAOIOffsetX.SetValue(0);
    camera.AutoFunctionAOIOffsetY.SetValue(0);
    camera.AutoFunctionAOIWidth.SetValue(camera.Width.GetMax());
    camera.AutoFunctionAOIHeight.SetValue(camera.Height.GetMax());

    // Set the target value for luminance control.
    // A value of 0.3 means that the target brightness is 30 % of the maximum brightness of the raw pixel value read out from the sensor.
    // A value of 0.4 means 40 % and so forth.
    camera.AutoTargetBrightness.SetValue(0.3);

    // We are trying GainAuto = Continuous.
    cout << "Trying 'GainAuto = Continuous'." << endl;
    cout << "Initial Gain = " << camera.Gain.GetValue() << endl;

    camera.GainAuto.SetValue(GainAuto_Continuous);

    // When "continuous" mode is selected, the parameter value is adjusted repeatedly while images are acquired.
    // Depending on the current frame rate, the automatic adjustments will usually be carried out for
    // every or every other image unless the camera’s micro controller is kept busy by other tasks.
    // The repeated automatic adjustment will proceed until the "once" mode of operation is used or
    // until the auto function is set to "off", in which case the parameter value resulting from the latest
    // automatic adjustment will operate unless the value is manually adjusted.
    for (int n = 0; n < 20; n++)            // For demonstration purposes, we will grab "only" 20 images.
    {
        GrabResultPtr_t ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult);
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage(1, ptrGrabResult);
#endif

        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep(100);
    }
    camera.GainAuto.SetValue(GainAuto_Off); // Switch off GainAuto.

    cout << "Final Gain = " << camera.Gain.GetValue() << endl << endl;
}


void AutoExposureOnce(Camera_t& camera)
{
    // Check whether auto exposure is available
    if ( !IsWritable( camera.ExposureAuto))
    {
        cout << "The camera does not support Exposure Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed area of interest (Image AOI).
    if (IsWritable(camera.OffsetX))
    {
        camera.OffsetX.SetValue(camera.OffsetX.GetMin());
    }
    if (IsWritable(camera.OffsetY))
    {
        camera.OffsetY.SetValue(camera.OffsetY.GetMin());
    }
    camera.Width.SetValue(camera.Width.GetMax());
    camera.Height.SetValue(camera.Height.GetMax());

    // Set the Auto Function AOI for luminance statistics.
    // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
    // luminance statistics.
    camera.AutoFunctionAOISelector.SetValue(AutoFunctionAOISelector_AOI1);
    camera.AutoFunctionAOIOffsetX.SetValue(0);
    camera.AutoFunctionAOIOffsetY.SetValue(0);
    camera.AutoFunctionAOIWidth.SetValue(camera.Width.GetMax());
    camera.AutoFunctionAOIHeight.SetValue(camera.Height.GetMax());

    // Set the target value for luminance control.
    // A value of 0.3 means that the target brightness is 30 % of the maximum brightness of the raw pixel value read out from the sensor.
    // A value of 0.4 means 40 % and so forth.
    camera.AutoTargetBrightness.SetValue(0.3);

    // Try ExposureAuto = Once.
    cout << "Trying 'ExposureAuto = Once'." << endl;
    cout << "Initial exposure time = ";
    cout << camera.ExposureTime.GetValue() << " us" << endl;

    // Set the exposure time ranges for luminance control.
    camera.AutoExposureTimeLowerLimit.SetValue(camera.AutoExposureTimeLowerLimit.GetMin());
    camera.AutoExposureTimeUpperLimit.SetValue(camera.AutoExposureTimeLowerLimit.GetMax());

    camera.ExposureAuto.SetValue(ExposureAuto_Once);

    // When the "once" mode of operation is selected, 
    // the parameter values are automatically adjusted until the related image property
    // reaches the target value. After the automatic parameter value adjustment is complete, the auto
    // function will automatically be set to "off", and the new parameter value will be applied to the
    // subsequently grabbed images.
    int n = 0;
    while (camera.ExposureAuto.GetValue() != ExposureAuto_Off)
    {
        GrabResultPtr_t ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult);
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage(1, ptrGrabResult);
#endif
        ++n;

        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep(100);

        //Make sure the loop is exited.
        if (n > 100)
        {
            throw RUNTIME_EXCEPTION( "The adjustment of auto exposure did not finish.");
        }
    }
    cout << "ExposureAuto went back to 'Off' after " << n << " frames." << endl;
    cout << "Final exposure time = ";
    cout << camera.ExposureTime.GetValue() << " us" << endl << endl;
}


void AutoExposureContinuous(Camera_t& camera)
{
    // Check whether the Exposure Auto feature is available.
    if ( !IsWritable( camera.ExposureAuto))
    {
        cout << "The camera does not support Exposure Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed area of interest (Image AOI).
    if (IsWritable(camera.OffsetX))
    {
        camera.OffsetX.SetValue(camera.OffsetX.GetMin());
    }
    if (IsWritable(camera.OffsetY))
    {
        camera.OffsetY.SetValue(camera.OffsetY.GetMin());
    }
    camera.Width.SetValue(camera.Width.GetMax());
    camera.Height.SetValue(camera.Height.GetMax());

    // Set the Auto Function AOI for luminance statistics.
    // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
    // luminance statistics.
    camera.AutoFunctionAOISelector.SetValue(AutoFunctionAOISelector_AOI1);
    camera.AutoFunctionAOIOffsetX.SetValue(0);
    camera.AutoFunctionAOIOffsetY.SetValue(0);
    camera.AutoFunctionAOIWidth.SetValue(camera.Width.GetMax());
    camera.AutoFunctionAOIHeight.SetValue(camera.Height.GetMax());

    // Set the target value for luminance control.
    // A value of 0.3 means that the target brightness is 30 % of the maximum brightness of the raw pixel value read out from the sensor.
    // A value of 0.4 means 40 % and so forth.
    camera.AutoTargetBrightness.SetValue(0.3);

    cout << "ExposureAuto 'GainAuto = Continuous'." << endl;
    cout << "Initial exposure time = ";
    cout << camera.ExposureTime.GetValue() << " us" << endl;

    camera.ExposureAuto.SetValue(ExposureAuto_Continuous);

    // When "continuous" mode is selected, the parameter value is adjusted repeatedly while images are acquired.
    // Depending on the current frame rate, the automatic adjustments will usually be carried out for
    // every or every other image, unless the camera’s microcontroller is kept busy by other tasks.
    // The repeated automatic adjustment will proceed until the "once" mode of operation is used or
    // until the auto function is set to "off", in which case the parameter value resulting from the latest
    // automatic adjustment will operate unless the value is manually adjusted.
    for (int n = 0; n < 20; n++)    // For demonstration purposes, we will use only 20 images.
    {
        GrabResultPtr_t ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult);
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage(1, ptrGrabResult);
#endif

        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep(100);
    }
    camera.ExposureAuto.SetValue(ExposureAuto_Off); // Switch off Exposure Auto.

    cout << "Final exposure time = ";
    cout << camera.ExposureTime.GetValue() << " us" << endl << endl;
}


void AutoWhiteBalance(Camera_t& camera)
{
    // Check whether the Balance White Auto feature is available.
    if ( !IsWritable( camera.BalanceWhiteAuto))
    {
        cout << "The camera does not support Balance White Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed area of interest (Image AOI).
    if (IsWritable(camera.OffsetX))
    {
        camera.OffsetX.SetValue(camera.OffsetX.GetMin());
    }
    if (IsWritable(camera.OffsetY))
    {
        camera.OffsetY.SetValue(camera.OffsetY.GetMin());
    }
    camera.Width.SetValue(camera.Width.GetMax());
    camera.Height.SetValue(camera.Height.GetMax());

    // Set the Auto Function AOI for white balance statistics.
    // Currently, AutoFunctionAOISelector_AOI2 is predefined to gather
    // white balance statistics.
    camera.AutoFunctionAOISelector.SetValue(AutoFunctionAOISelector_AOI2);
    camera.AutoFunctionAOIOffsetX.SetValue(0);
    camera.AutoFunctionAOIOffsetY.SetValue(0);
    camera.AutoFunctionAOIWidth.SetValue(camera.Width.GetMax());
    camera.AutoFunctionAOIHeight.SetValue(camera.Height.GetMax());

    cout << "Trying 'BalanceWhiteAuto = Once'." << endl;
    cout << "Initial balance ratio: ";
    camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
        cout << "R = " << camera.BalanceRatio.GetValue() << "   ";
    camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Green);
        cout << "G = " << camera.BalanceRatio.GetValue() << "   ";
    camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Blue);
        cout << "B = " << camera.BalanceRatio.GetValue() << endl;

    camera.BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Once);

    // When the "once" mode of operation is selected, 
    // the parameter values are automatically adjusted until the related image property
    // reaches the target value. After the automatic parameter value adjustment is complete, the auto
    // function will automatically be set to "off" and the new parameter value will be applied to the
    // subsequently grabbed images.
    int n = 0;
    while (camera.BalanceWhiteAuto.GetValue() != BalanceWhiteAuto_Off)
    {
        GrabResultPtr_t ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult);
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage(1, ptrGrabResult);
#endif
        ++n;

        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep(100);

        //Make sure the loop is exited.
        if (n > 100)
        {
            throw RUNTIME_EXCEPTION( "The adjustment of auto white balance did not finish.");
        }
    }
    cout << "BalanceWhiteAuto went back to 'Off' after ";
    cout << n << " frames." << endl;
    cout << "Final balance ratio: ";
    camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Red);
    cout << "R = " << camera.BalanceRatio.GetValue() << "   ";
    camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Green);
    cout << "G = " << camera.BalanceRatio.GetValue() << "   ";
    camera.BalanceRatioSelector.SetValue(BalanceRatioSelector_Blue);
    cout << "B = " << camera.BalanceRatio.GetValue() << endl;
}


bool IsColorCamera(Camera_t& camera)
{
    GenApi::NodeList_t Entries;
    camera.PixelFormat.GetEntries(Entries);
    bool Result = false;

    for (size_t i = 0; i < Entries.size(); i++)
    {
        GenApi::INode *pNode = Entries[i];
        if (IsAvailable(pNode->GetAccessMode()))
        {
            GenApi::IEnumEntry *pEnum = dynamic_cast<GenApi::IEnumEntry *>(pNode);
            const GenICam::gcstring sym(pEnum->GetSymbolic());
            if (sym.find(GenICam::gcstring("Bayer")) != GenICam::gcstring::_npos())
            {
                Result = true;
                break;
            }
        }
    }
    return Result;
}
