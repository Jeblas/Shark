#include <string>
#include <iomanip>
#include <sstream>

#include <opencv2/highgui.hpp>
#include <opencv2/video/tracking.hpp>

#include <sl/Camera.hpp>
#include <opencv2/gpu/gpu.hpp>

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

using namespace std;
using namespace cv;
using namespace sl;
using namespace boost::interprocess;


typedef allocator<int, managed_shared_memory::segment_manager>  ShmemAllocator;
typedef vector<cv::Mat, ShmemAllocator> cvMatVect;

struct shm_remove {
         shm_remove() {
            shared_memory_object::remove("SharedMemoryFrames");
            shared_memory_object::remove("SharedMemoryState"); }
         ~shm_remove(){
            shared_memory_object::remove("SharedMemoryFrames");
            shared_memory_object::remove("SharedMemoryState"); }
 } remover;


ERROR_CODE err;
RuntimeParameters runtime_parameters;
int main(int argc, char *argv[])
{
    Camera zed;
    //ZED initialize and Params
    InitParameters init_params;

    init_params.camera_resolution = RESOLUTION_VGA;

    init_params.camera_fps = 15;
    init_params.coordinate_units = UNIT_MILLIMETER;
    init_params.depth_minimum_distance = 300;
	init_params.depth_mode = DEPTH_MODE_PERFORMANCE;
	cv::Mat state(10, 1, CV_32F);

	err = zed.open(init_params);
	cout <<"zed opened\n";
	if (err != SUCCESS)
	{
	std::cout << errorCode2str(err) << std::endl;
	zed.close();
	return EXIT_FAILURE; // quit if an error occurred
	}
	zed.setDepthMaxRangeValue(10000);
	runtime_parameters.sensing_mode = SENSING_MODE_FILL;
	sl::Mat inFrame_zl, inFrame_zr, inFrame_zd, inFrame_zq, inFrame_zdm, inFrame_zcm,  inFrame_zc;
	cv::Mat inFrame_l, inFrame_d, state;
	
	// Boost interprocess config
    // could do segment*(open_or_create) to connect if not created
	 managed_shared_memory segmentFrames(open_or_create, "SharedMemoryFrames", 65536);
     managed_shared_memory segmentState(open_or_create,"SharedMemoryState");

	//Initialize shared memory STL-compatible allocator
	const ShmemAllocator alloc_inst (segmentFrames.get_segment_manager());
    //const ShmemAllocator alloc_inst_read(segmentRead.get_segment_manager());


	//Construct a vector named "inframeData" in shared memory with argument alloc_inst
	cvMatVect *inframeData = segmentFrames.construct<cvMatVect>("inframes")(alloc_inst);
    cvMatVect *stateData = segmentState.find<cvMatVect>("state").first
    //////////////////////////////

	while(true)
	{
		err = zed.grab(runtime_parameters);
		err = zed.retrieveImage(inFrame_zl, VIEW_LEFT);
		inFrame_l = cv::Mat(inFrame_zl.getHeight(), inFrame_zl.getWidth(), CV_8UC4, inFrame_zl.getPtr<sl::uchar1>(sl::MEM_CPU));
		err = zed.retrieveMeasure(inFrame_zdm);
		inFrame_d = cv::Mat(inFrame_zdm.getHeight(), inFrame_zdm.getWidth(), CV_8UC4, inFrame_zdm.getPtr<sl::uchar1>(sl::MEM_CPU));
		
		//TODO send inFrame_l and inFrame_d to Shark

        // Might need to do a deep copy use Mat::clone()
        // something something cv::Mat::release()
		inframeData->operator[](0) = inFrame_l;
		inframeData->operator[](1) = inFrame_d;
		
		//TODO read in state from Shark
		state = stateData->operator[](0);
        //idk if cv:mat needs to be formated of can be sent to cout
		cout << state << endl;
		
	}

    // close sharedMemoryStuff
}