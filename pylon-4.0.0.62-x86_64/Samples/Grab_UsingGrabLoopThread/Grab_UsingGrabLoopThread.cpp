// Grab_UsingGrabLoopThread.cpp
/*
   This sample illustrates how to grab and process images using the grab loop thread
   provided by the Instant Camera class.
*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

// Include files used by samples.
#include "../include/ConfigurationEventPrinter.h"
#include "../include/ImageEventPrinter.h"

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

//Example of an image event handler.
class CSampleImageEventHandler : public CImageEventHandler
{
public:
    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult)
    {
#ifdef PYLON_WIN_BUILD
        // Display the image
        Pylon::DisplayImage(1, ptrGrabResult);
#endif

        cout << "CSampleImageEventHandler::OnImageGrabbed called." << std::endl;
        cout << std::endl;
        cout << std::endl;
    }
};

int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;

    // Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
    // is initialized during the lifetime of this object.
    Pylon::PylonAutoInitTerm autoInitTerm;

    try
    {
        // Create an instant camera object for the camera device found first.
        CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());

        // Register the standard configuration event handler for enabling software triggering.
        // The software trigger configuration handler replaces the default configuration
        // as all currently registered configuration handlers are removed by setting the registration mode to RegistrationMode_ReplaceAll.
        camera.RegisterConfiguration( new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);

        // For demonstration purposes only, add a sample configuration event handler to print out information
        // about camera use.
        camera.RegisterConfiguration( new CConfigurationEventPrinter, RegistrationMode_Append, Cleanup_Delete);

        // The image event printer serves as sample image processing.
        // When using the grab loop thread provided by the Instant Camera object, an image event handler processing the grab
        // results must be created and registered.
        camera.RegisterImageEventHandler( new CImageEventPrinter, RegistrationMode_Append, Cleanup_Delete);

        // For demonstration purposes only, register another image event handler.
        camera.RegisterImageEventHandler( new CSampleImageEventHandler, RegistrationMode_Append, Cleanup_Delete);

        // Start the grabbing using the grab loop thread, by setting the grabLoopType parameter
        // to GrabLoop_ProvidedByInstantCamera. The grab results are delivered to the image event handlers.
        // The GrabStrategy_OneByOne default grab strategy is used.
        camera.StartGrabbing( GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);

        cerr << endl << "Enter \"t\" to trigger the camera or \"e\" to exit and press enter? (t/e)" << endl << endl;

        // Wait for user input to trigger the camera or exit the program.
        // The grabbing is stopped, the device is closed and destroyed automatically when the camera object goes out of scope.
        char key;
        do
        {
            cin.get(key);
            if ( (key == 't' || key == 'T'))
            {
                // Execute the software trigger. Wait up to 100 ms for the camera to be ready for trigger.
                if ( camera.WaitForFrameTriggerReady( 100, TimeoutHandling_ThrowException))
                {
                    camera.ExecuteSoftwareTrigger();
                }
            }
        }
        while ( (key != 'e') && (key != 'E'));
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

