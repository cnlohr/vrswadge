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
#include <unistd.h>

int swadgeshm_video;
int swadgeshm_input;

cnovr_shader * shaderOLED;
cnovr_shader * shaderLED;
cnovr_shader * shaderLines;

cnovr_model * swadgescreen;
cnovr_model * swadgeleds;
cnovr_model * swadgebuttons;
cnovr_model * swadgemodel;

cnovr_texture * oled_texture;

cnovrfocus_capture modelcapture;

cnovrfocus_capture buttoncapture;

struct staticstore
{
	int initialized;
	cnovr_pose    swadgemodelpose;
} * store;

uint32_t * swadgeshm_video_data;
uint8_t * swadgeshm_input_data;
uint32_t buttondowns[5];

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

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	CNOVRRender( shaderLED );
	glUniform4iv( CNOVRMAPPEDUNIFORMPOS( 19 ), 4, swadgeshm_video_data+4 );
	CNOVRRender( swadgeleds );

	CNOVRRender( shaderLED );
	glUniform4iv( CNOVRMAPPEDUNIFORMPOS( 19 ), 4, buttondowns );
	CNOVRRender( swadgebuttons );
}

int ButtonFocusEvent( int event, cnovrfocus_capture * cap, cnovrfocus_properties * prop, int buttoninfo )
{
	int button = prop->collide_results.whichvert/6;
	switch( event )
	{
		case CNOVRF_DOWNFOCUS: case CNOVRF_DOWNNOFOCUS:
			buttondowns[button] = 0xffff0000;
			swadgeshm_input_data[button] = 1;
			break;
		case CNOVRF_UPFOCUS: case CNOVRF_UPNOFOCUS:
			buttondowns[button] = 0xffffffff;
			swadgeshm_input_data[button] = 0;
			break;
	}
	return 0;
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
	shaderLED = CNOVRShaderCreate( "assets/led" );
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
	CNOVRModelAppendMesh( swadgescreen, 1, 1, 1, (cnovr_point3d){.63,.35,1}, &poseofs, 0 );
		//cnovr_model * m, int rows, int cols, int flipv, cnovr_point3d size, cnovr_pose * poseofs_optional, cnovr_point4d * extradata );

	oled_texture = CNOVRTextureCreate( oscreenw, oscreenh, 4 ); //Set to all 0 to have the load control these details.
	oled_texture->bDisableTextureDataFree = 1;
	CNOVRModelSetNumTextures( swadgescreen, 1 );
	swadgescreen->pTextures[0] = oled_texture;
	RemoveTCCDeleteTag( oled_texture ); //Will be cleared when object is destroyed.

	cnovr_point4d pointextradata[6];

	{
		swadgeleds = CNOVRModelCreate( 0, GL_TRIANGLES );
		cnovr_point3d offsets[6] = { { -.63, .85, .1 }, { -.67, 1.68, .1 }, { -.35, 2.5, .1} , { .44, 2.5, .1 }, { .76,1.68,.1}, {.72,.85,.1} };
		for( i = 0; i < 6; i++ )
		{
			pose_make_identity( &poseofs );
			add3d( poseofs.Pos, poseofs.Pos, offsets[i] );
			pointextradata[i][0] = i + 0.5;
			CNOVRModelAppendMesh( swadgeleds, 1, 1, 1, (cnovr_point3d){.2,.2,.2}, &poseofs, pointextradata+i );
		}
	}
	{
		swadgebuttons = CNOVRModelCreate( 0, GL_TRIANGLES );
		cnovr_point3d offsets[5] = { { -.95, -1.08, .25 }, {-.64, -.76,.25}, { -.31, -1.08, .25 }, { -.64, -1.4, .25} , { .68, -1.08, .25 } };
		for( i = 0; i < 5; i++ )
		{
			buttondowns[i] = 0xffffffff;
			pose_make_identity( &poseofs );
			add3d( poseofs.Pos, poseofs.Pos, offsets[i] );
			pointextradata[i][0] = i + 0.5;
			cnovr_point3d size = { .15, .15, .15 };
			if( i == 4 )
			{
				scale3d( size, size, 2. );
			}
			swadgebuttons->pose = &store->swadgemodelpose;
			CNOVRModelAppendMesh( swadgebuttons, 1, 1, 1, size, &poseofs, pointextradata+i );
		}
		buttoncapture.tag = 0;
		buttoncapture.opaque = swadgebuttons;
		buttoncapture.cb = ButtonFocusEvent;
		CNOVRModelSetInteractable( swadgebuttons, &buttoncapture );


	}


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
	if( swadgeshm_video<= 0 || swadgeshm_input<= 0 )
	{
		printf( "Error: Swadge is not running.\n" );
		exit( 5 );
	}
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


