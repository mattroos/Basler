// ParametrizeCamera_LoadAndSave.cpp
/*
   This sample application demonstrates how to save or load the features of a camera
   to or from a file.
*/

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// The name of the pylon feature stream file.
const char Filename[] = "NodeMap.pfs";

int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;

    // Automagically call PylonInitialize and PylonTerminate to ensure that the pylon runtime
    // system is initialized during lifetime of this object.
    Pylon::PylonAutoInitTerm autoInitTerm;

    try
    {
        // Create an instant camera object with the camera device found first.
        CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        // Open the camera.
        camera.Open();

        cout << "Saving camera's node map to file..."<< endl;
        // Save the content of the camera's node map into the file.
        CFeaturePersistence::Save( Filename, &camera.GetNodeMap() );
        // --------------------------------------------------------------------

        // Just for demonstration, read the content of the file back to the camera's node map with enabled validation.
        cout << "Reading file back to camera's node map..."<< endl;
        CFeaturePersistence::Load( Filename, &camera.GetNodeMap(), true );

        // Close the camera.
        camera.Close();
    }
    catch (GenICam::GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Comment the following two lines to disable waiting on exit
    cerr << endl << "Press Enter to exit." << endl;
    while( cin.get() != '\n');

    return exitCode;
}

