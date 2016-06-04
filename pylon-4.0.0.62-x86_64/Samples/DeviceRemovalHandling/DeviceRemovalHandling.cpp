// DeviceRemovalHandling.cpp
/*
    This sample program demonstrates how to be informed about the removal of a camera device.
    It also shows how to reconnect to a removed device.

    Attention:
    If you build this sample and run it under a debugger using a GigE camera device, pylon will set the heartbeat
    timeout to 5 minutes. This is done to allow debugging and single stepping of the code without
    the camera thinking we're hung because we don't send any heartbeats.
    Accordingly, it would take 5 minutes for the application to notice the disconnection of a GigE device.

    To work around this, the CHeatbeatHelper class is used to control the HeartbeatTimeout.
    Just before waiting for the removal, it will set the timeout to 1000 ms.
*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#include "../include/ConfigurationEventPrinter.h"

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;


// Simple helper class to set the HeartbeatTimeout safely.
class CHearbeatHelper
{
    public:
        explicit CHearbeatHelper(CInstantCamera& camera)
                : m_pHeartbeatTimeout(NULL)
        {
            // m_pHeartbeatTimeout may be NULL
            m_pHeartbeatTimeout = camera.GetTLNodeMap().GetNode("HeartbeatTimeout");
        }

        bool SetValue(int64_t NewValue)
        {
            // Do nothing if no heartbeat feature is available.
            if (!m_pHeartbeatTimeout.IsValid())
                return false;

            // Apply the increment and cut off invalid values if neccessary.
            int64_t correctedValue = NewValue - (NewValue % m_pHeartbeatTimeout->GetInc());

            m_pHeartbeatTimeout->SetValue(correctedValue);
            return true;
        }

        bool SetMax()
        {
            // Do nothing if no heartbeat feature is available.
            if (!m_pHeartbeatTimeout.IsValid())
                return false;

            int64_t maxVal = m_pHeartbeatTimeout->GetMax();
            return SetValue(maxVal);
        }

    protected:
        GenApi::CIntegerPtr m_pHeartbeatTimeout; // Pointer to the node, will be NULL if no node exists.
};

// When using Device Specific Instant Camera classes there are specific Configuration event handler classes available which can be used, for example
// Pylon::CBaslerGigEConfigurationEventHandler or Pylon::CBasler1394ConfigurationEventHandler
//Example of a configuration event handler that handles device removal events.
class CSampleConfigurationEventHandler : public Pylon::CConfigurationEventHandler
{
public:
    // This method is called from a different thread when the camera device removal has been detected.
    void OnCameraDeviceRemoved( CInstantCamera& /*camera*/)
    {
        // Print two new lines, just for improving printed output.
        cout << endl << endl;
        cout << "CSampleConfigurationEventHandler::OnCameraDeviceRemoved called." << std::endl;
    }
};

// Time to wait in quarters of seconds.
static const uint32_t c_loopCounterInitialValue = 20 * 4;

int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;

    // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
    // is initialized during the lifetime of this object.
    Pylon::PylonAutoInitTerm autoInitTerm;

    try
    {
        // Declare a local counter used for waiting.
        int loopCount = 0;

        // Get the transport layer factory.
        CTlFactory& tlFactory = CTlFactory::GetInstance();

        // Create an instant camera object with the camera device found first.
        CInstantCamera camera( tlFactory.CreateFirstDevice());

        // Print the camera information.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;
        cout << "Friendly Name: " << camera.GetDeviceInfo().GetFriendlyName() << endl;
        cout << "Full Name    : " << camera.GetDeviceInfo().GetFullName() << endl;
        cout << "SerialNumber : " << camera.GetDeviceInfo().GetSerialNumber() << endl;
        cout << endl;

        // For demonstration purposes only, register another configuration event handler that handles device removal.
        camera.RegisterConfiguration( new CSampleConfigurationEventHandler, RegistrationMode_Append, Cleanup_Delete);

        // For demonstration purposes only, add a sample configuration event handler to print out information
        // about camera use.
        camera.RegisterConfiguration( new CConfigurationEventPrinter, RegistrationMode_Append, Cleanup_Delete);

        // Open the camera. Camera device removal is only detected while the camera is open.
        camera.Open();

        // Now, try to detect that the camera has been removed:

        // Ask the user to disconnect a device
        loopCount = c_loopCounterInitialValue;
        cout << endl << "Please disconnect the device (timeout " << loopCount / 4 << "s) " << endl;

        /////////////////////////////////////////////////// don't single step beyond this line  (see comments above)

        // Before testing the callbacks, we manually set the heartbeat timeout to a short value when using GigE cameras.
        // Since for debug versions the heartbeat timeout has been set to 5 minutes, it would take up to 5 minutes
        // until detection of the device removal.
        CHearbeatHelper heartbeatHelper(camera);
        heartbeatHelper.SetValue(1000);  // 1000 ms timeout

        try
        {
            // Get a camera parameter using generic parameter access.
            GenApi::CIntegerPtr width(camera.GetNodeMap().GetNode("Width"));

            // The following loop accesses the camera. It could also be a loop that is
            // grabbing images. The device removal is handled in the exception handler.
            while ( loopCount > 0)
            {
                // Print a "." every few seconds to tell the user we're waiting for the callback.
                if (--loopCount % 4 == 0)
                {
                    cout << ".";
                    cout.flush();
                }
                WaitObject::Sleep(250);

                // Change the width value in the camera depending on the loop counter.
                // Any access to the camera like setting parameters or grabbing images
                // will fail throwing an exception if the camera has been disconnected.
                width->SetValue( width->GetMax() - (width->GetInc() * (loopCount % 2)));
            }

        }
        catch (GenICam::GenericException &e)
        {
            if ( camera.IsCameraDeviceRemoved())
            {
                // The camera device has been removed. This caused the exception.
                cout << endl;
                cout << "The camera has been removed from the PC." << endl;
                cout << "The camera device removal triggered an exception:" << endl
                    << e.GetDescription() << endl;
            }
            else
            {
                // An unexpected error has occurred.
                // In this example it is handled by exiting the program.
                throw;
            }
        }

        if ( !camera.IsCameraDeviceRemoved())
            cout << endl << "Timeout expired" << endl;

        /////////////////////////////////////////////////// Safe to use single stepping (see comments above).

        // Now try to find the detached camera after it has been attached again:

        // Create a device info object for remembering the camera properties.
        CDeviceInfo info;

        // Remember the camera properties that allow detecting the same camera again.
        info.SetDeviceClass( camera.GetDeviceInfo().GetDeviceClass());
        info.SetSerialNumber( camera.GetDeviceInfo().GetSerialNumber());

        // Destroy the Pylon Device representing the detached camera device.
        // It cannot be used anymore.
        camera.DestroyDevice();

        // Ask the user to connect the same device.
        loopCount = c_loopCounterInitialValue;
        cout << endl << "Please connect the same device to the PC again (timeout " << loopCount / 4 << "s) " << endl;

        // Create a filter containing the CDeviceInfo object info which describes the properties of the device we are looking for.
        DeviceInfoList_t filter;
        filter.push_back( info);

        for ( ; loopCount > 0; --loopCount)
        {
            // Print a . every few seconds to tell the user we're waiting for the camera to be attached
            if ( loopCount % 4 == 0)
            {
                cout << ".";
                cout.flush();
            }

            // Try to find the camera we are looking for.
            DeviceInfoList_t devices;
            if ( tlFactory.EnumerateDevices(devices, filter) > 0 )
            {
                // Print two new lines, just for improving printed output.
                cout << endl << endl;

                // The camera has been found. Create and attach it to the Instant Camera object.
                camera.Attach( tlFactory.CreateDevice( devices[0]));
                //Exit waiting
                break;
            }

            WaitObject::Sleep(250);
        }

        // If the camera has been found.
        if ( camera.IsPylonDeviceAttached())
        {
            // Print the camera information.
            cout << endl;
            cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;
            cout << "Friendly Name: " << camera.GetDeviceInfo().GetFriendlyName() << endl;
            cout << "Full Name    : " << camera.GetDeviceInfo().GetFullName() << endl;
            cout << "SerialNumber : " << camera.GetDeviceInfo().GetSerialNumber() << endl;
            cout << endl;

            // All configuration objects and other event handler objects are still registered.
            // The configuration objects will parameterize the camera device and the instant
            // camera will be ready for operation again.

            // Open the camera.
            camera.Open();

            // Now the Instant Camera object can be used as before.
        }
        else // Timeout
        {
            cout << endl << "Timeout expired." << endl;
        }
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
