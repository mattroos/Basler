// ParametrizeCamera_LoadAndSaveSequenceSets.cpp
/*
    This sample demonstrates how to save and load the settings for
    several sequence sets to or from a file that are used with the
    sequencer feature for Basler cameras.
*/


// Include files to use the PYLON API
#include <pylon/PylonIncludes.h>
#include <pylon/PylonUtilityIncludes.h>

#if defined ( USE_GIGE )
// Settings for use with Basler GigE cameras
#include <pylon/gige/BaslerGigEInstantCamera.h>
#include "log4cpp/Portability.hh"
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
using namespace Basler_GigECameraParams;
using namespace Basler_GigEStreamParams;

#else

#error camera type is not specified. For define USE_GIGE for using GigE cameras

#endif

// Namespace for using pylon objects
using namespace Pylon;
// Namespace for using cout
using namespace std;

int main(int argc, char* argv[])
{
    // The exit code of the sample application.
    int exitCode = 0;

    // Automagically call PylonInitialize and PylonTerminate to ensure that the pylon runtime
    // system is initialized during lifetime of this object
    Pylon::PylonAutoInitTerm autoInitTerm;

    try
    {
        // Only look for cameras supported by Camera_t.
        CDeviceInfo info;
        info.SetDeviceClass( Camera_t::DeviceClass());

        // Create an instant camera object with the first found camera device matching the specified device class.
        Camera_t camera( CTlFactory::GetInstance().CreateFirstDevice( info));

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        // Open the camera.
        camera.Open();

        // Check whether the camera supports the sequencer feature.
        if (GenApi::IsAvailable(camera.SequenceEnable))
        {
            // Limit number of sequence sets to max count reported by camera.
            int64_t cntSequences = 3;
            if (cntSequences > camera.SequenceSetTotalNumber.GetMax())
            {
                cntSequences = camera.SequenceSetTotalNumber.GetMax();
            }

            // Set the number of sequence sets to be used.
            camera.SequenceSetTotalNumber.SetValue(cntSequences);

            cout << "Saving camera's sequence settings to file..."<< endl;

            // Save the content of the camera's nodemap for each sequence set index number into separate files.
            for (int index = 0; index < cntSequences; ++index)
            {
                // Select a sequence set index number.
                camera.SequenceSetIndex = index;

                // Load the sequence parameter values related to the selected sequence set into the active set.
                camera.SequenceSetLoad.Execute();

                // Save all parameter settings to file.
                std::ostringstream filename;
                filename <<  "NodeMap_"<< index << ".pfs";
                cout << "Saving SequenceSetIndex " << index << " to file '" << filename.str() << "'" << endl;
                CFeaturePersistence::Save( filename.str().c_str(), &camera.GetNodeMap() );
            }

            // --------------------------------------------------------------------

            // Just for demonstration purposes, read the content of the file back to the camera's nodemap with validation on.
            cout << "Reading values back to camera's nodemap..."<< endl;

            for (int index = 0; index < cntSequences; ++index)
            {
                // Load settings from file.
                std::ostringstream filename;
                filename <<  "NodeMap_" << index << ".pfs";
                cout << "Loading sequence set index number "<< index << " from file '" << filename.str() << "'" << endl;
                CFeaturePersistence::Load( filename.str().c_str(), &camera.GetNodeMap(), true );

                // Set the sequence set index number of the sequence set where to save the sequence parameter values from the active set.
                camera.SequenceSetIndex = index;

                // Save the sequence set parameter values.
                camera.SequenceSetStore.Execute();
            }
        }
        else
        {
            cout << "The sequencer feature is not available for this camera."<< endl;
        }

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

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press Enter to exit." << endl;
    while( cin.get() != '\n');

    return exitCode;
}
