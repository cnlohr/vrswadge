#include <cnovrtcc.h>
#include <cnovrparts.h>
#include <cnovrfocus.h>
#include <cnovr.h>
#include <cnovrutil.h>
#include <stdlib.h>
#include <string.h>

//For SHM_OPEN
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */


int swadgeshm_video;
int swadgeshm_input;

cnovr_shader * shaderOLED;
cnovr_shader * shaderLines;

cnovr_model * swadgemodel;
cnovr_model * swadgescreen;

cnovr_texture * oled_texture;

cnovrfocus_capture modelcapture;

struct staticstore
{
	int initialized;
	cnovr_pose    swadgemodelpose;
} * store;

uint32_t * swadgeshm_video_data;
uint8_t * swadgeshm_input_data;


int oscreenw, oscreenh;


void init( const char * identifier )
{
	ovrprintf( "Example init %s\n", identifier );
}

void UpdateFunction( void * tag, void * opaquev )
{
	static double start;
	static double time_of_save;
	double now = OGGetAbsoluteTime();
	if( start < 1 ) start = now;

	if( now-time_of_save > 1 )
	{
		CNOVRNamedPtrSave( "vrswadgestore" );
		time_of_save = now;
	}

	// Do nothing.

	return;
}


void RenderFunction( void * tag, void * opaquev )
{
	int i;
	CNOVRTextureLoadDataNow( oled_texture, oscreenw, oscreenh, 4, 0, ((uint32_t*)swadgeshm_video_data)+(64/4), 1 );

	CNOVRRender( shaderLines );
	CNOVRRender( swadgemodel );
	CNOVRRender( shaderOLED );
	CNOVRRender( swadgescreen );
}


static void example_scene_setup( void * tag, void * opaquev )
{
	printf( "+++ Example scene setup\n" );
	int i;

	printf( "STORE: %p %d\n", store, store->initialized );
	if( !store->initialized )
	{
		pose_make_identity( &store->swadgemodelpose );
		store->swadgemodelpose.Scale = 1;
		store->initialized = 1;
	}

	shaderOLED = CNOVRShaderCreate( "assets/oled" );
	shaderLines = CNOVRShaderCreate( "assets/fakelines" );

	swadgemodel = CNOVRModelCreate( 0, GL_TRIANGLES );
	swadgemodel->pose = &store->swadgemodelpose;
	CNOVRModelLoadFromFileAsync( swadgemodel, "swadge.obj:barytc" );


	modelcapture.tag = 0;
	modelcapture.opaque = swadgemodel;
	modelcapture.cb = CNOVRFocusDefaultFocusEvent;
	CNOVRModelSetInteractable( swadgemodel, &modelcapture );

	swadgescreen = CNOVRModelCreate( 0, GL_TRIANGLES );

	cnovr_pose poseofs;
	pose_make_identity( &poseofs );
	poseofs.Pos[0] += .05;
	poseofs.Pos[1] += .11;
	poseofs.Pos[2] -= .8;
	CNOVRModelAppendMesh( swadgescreen, 1, 1, 1, (cnovr_point3d){.65,.35,1}, &poseofs, 0 );
		//cnovr_model * m, int rows, int cols, int flipv, cnovr_point3d size, cnovr_pose * poseofs_optional, cnovr_point4d * extradata );

#if 1
	oled_texture = CNOVRTextureCreate( oscreenw, oscreenh, 4 ); //Set to all 0 to have the load control these details.
	oled_texture->bDisableTextureDataFree = 1;
	CNOVRModelSetNumTextures( swadgescreen, 1 );
	swadgescreen->pTextures[0] = oled_texture;
	RemoveTCCDeleteTag( oled_texture ); //Will be cleared when object is destroyed.
#endif

	UpdateFunction(0,0);
	CNOVRListAdd( cnovrLUpdate, 0, UpdateFunction );
	CNOVRListAdd( cnovrLRender2, 0, RenderFunction );
	//CNOVRListAdd( cnovrLCollide, 0, CollideFunction );
	printf( "+++ Example scene setup complete\n" );
}


void start( const char * identifier )
{
	store = CNOVRNamedPtrData( "vrswadgestore", 0, sizeof( *store ) + 1024 );

	swadgeshm_video = shm_open("/swadgevideo", O_RDWR, 0644);
	swadgeshm_input = shm_open("/swadgeinput", O_RDWR, 0644);
	swadgeshm_input_data = mmap(0,10, PROT_READ | PROT_WRITE, MAP_SHARED, swadgeshm_input, 0);
	swadgeshm_video_data = mmap(0,1024*1024, PROT_READ | PROT_WRITE, MAP_SHARED, swadgeshm_video, 0);

	oscreenw = swadgeshm_video_data[0];
	oscreenh = swadgeshm_video_data[1];
	printf( "SWADGE SCREEN DIMENSIONS: %d %d\n", oscreenw, oscreenh );

	CNOVRJobTack( cnovrQPrerender, example_scene_setup, 0, 0, 0 );
	printf( "=== Example start %s(%p) + %p %p\n", identifier, identifier );
}

void stop( const char * identifier )
{
	munmap( swadgeshm_input_data, 10);
	munmap( swadgeshm_video_data, 1024*1024);
	close( swadgeshm_video );
	close( swadgeshm_input );

	printf( "=== End Example stop\n" );
}


