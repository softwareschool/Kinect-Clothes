#include <list>
#include "GL/glut.h"
#include "GL/gl.h"
#include "../../src/skeletonjelly.hpp"

#define WINDOW_X 800
#define WINDOW_Y 600

#define ROOM_X 4.0f // m
#define ROOM_Y 3.0f // m

#define GRID_SIZE 0.25f // 25 cm

#define SCALE(x) ((x) / 1000.0f)

bool g_running = false;
Kinect g_kinect;

char g_message[64] = {0};
char g_coords[64] = {0};
char g_leftHand[64] = {0};
char g_rightHand[64] = {0};
XnUInt32XYPair res;

unsigned char* imageBuffer;


const KinectUser *g_userData = NULL;

static const char *MESSAGES[] =
{
	"Found user",
	"Lost user",
	"Pose detected",
	"Calibration started...",
	"TRACKING",
	"Calibration failed"
};

void convertToProjCoordinates(XnSkeletonJointPosition &joint)
{
	xn::DepthGenerator* depth = g_kinect.Depth();
	depth->ConvertRealWorldToProjective(1,&joint.position,&joint.position);

	joint.position.X /= res.X;
	joint.position.Y /= res.Y;

}


void kinect_status(Kinect *k, Kinect::CallbackType cb_type, XnUserID id, void *data)
{
	_snprintf(g_message, 64, "User [%d]: %s", id, MESSAGES[cb_type]);
	//_snprintf("%s\n", g_message);

	if (cb_type == Kinect::CB_NEW_USER && id == 1)
	{
		g_userData = k->getUserData(id);
	}
}

void glPrintString(void *font, char *str)
{
	int i,l = strlen(str);

	for(i=0; i<l; i++)
	{
		glutBitmapCharacter(font,*str++);
	}
}

void drawRealImage()
{
	g_kinect.renderImage(imageBuffer, 0);

	XnUInt32XYPair res;

	res = g_kinect.getDepthResolution();

	glColor3f(1,1,1);

	glBindTexture(GL_TEXTURE_2D, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, res.X, res.Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageBuffer);

	glBegin(GL_QUADS);
		glTexCoord2f(0,1);
		glVertex2f(-1,-1);

		glTexCoord2f(1,1);
		glVertex2f(1,-1);

		glTexCoord2f(1,0);
		glVertex2f(1,1);

		glTexCoord2f(0,0);
		glVertex2f(-1,1);
	glEnd();

}

void drawJoint(XnSkeletonJointPosition& joint)
{
	convertToProjCoordinates(joint);

	glColor3f(1.0f, 1.0f, 1.0f);
			
	glBegin(GL_POINTS);
	glVertex3f(joint.position.X, joint.position.Y, .1f);
	glEnd();
}

void drawTracking()
{
	glOrtho(0, 1.0, 1.0, 0.0f, -1.0, 1.0);

	glPointSize(16.0f);
	glLineWidth(8.0f);

	if (g_userData)
	{
		const XnPoint3D *com = &g_userData->centerOfMass;

    	glColor3f(0.66, 0.33, 0.33);
		if (g_userData->status & Kinect::USER_TRACKING)
        	glColor3f(0.33, 0.66, 0.33);

		glBegin(GL_POINTS);
			glVertex3f(com->X, com->Y, 0.1f);
		glEnd();

		_snprintf(g_coords, 64, "CoM: (%0.4f, %0.4f, %0.4f)\n", com->X, com->Y, com->Z);

        if (g_kinect.userStatus() & Kinect::USER_TRACKING)
		{
			XnSkeletonJointPosition joint;
			for(int i=0;i<g_kinect.KINECT_JOINT_MAX;++i)
			{
				joint = g_userData->joints[i];
				printf("%d: %f,%f\n", i, joint.position.X, joint.position.Y);
				drawJoint(joint);
			}
			printf("\n");
		}
	}
}

void drawHUD()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, WINDOW_X, 0, WINDOW_Y, -1.0, 1.0);
	glDisable(GL_DEPTH_TEST); 

	glColor3f(1, 1, 1);

	glRasterPos2i(10, 10);
	glPrintString(GLUT_BITMAP_HELVETICA_18, g_message);

	glRasterPos2i(10, 30);
	glPrintString(GLUT_BITMAP_HELVETICA_18, g_coords);

	glRasterPos2i(10, 50);
	glPrintString(GLUT_BITMAP_HELVETICA_18, g_leftHand);

	glRasterPos2i(10, 70);
	glPrintString(GLUT_BITMAP_HELVETICA_18, g_rightHand);

	glEnable(GL_DEPTH_TEST); 
	glPopMatrix();
}


// this function is called each frame
void glutDisplay()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup the OpenGL viewpoint
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST); 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawRealImage();

	glDisable(GL_TEXTURE_2D);

	drawTracking();
	drawHUD();

	glutSwapBuffers();
}

void glutIdle()
{
	static int time = 0;

	int now = glutGet(GLUT_ELAPSED_TIME);

	g_kinect.tick(now - time);
	glutPostRedisplay();

	time = now;
}

void glutKeyboard (unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(0);
	}
}
void glInit (int *pargc, char **argv)
{
	glutInit(pargc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_X, WINDOW_Y);
	glutCreateWindow ("SkeletonJelly Debug");
	glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);

	glClearColor(1.0f, 153.0f / 255.0f, .2f, 1.0);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

int main(int argc, char **argv)
{
	g_kinect.setEventCallback(kinect_status, NULL);
	g_kinect.setRenderFormat(Kinect::RENDER_RGBA);
	g_kinect.setTicksPerSecond(30);
	g_kinect.init(Kinect::SENSOR_VGA_30FPS, Kinect::SENSOR_VGA_30FPS);

	glInit(&argc, argv);

	res = g_kinect.getDepthResolution();

	int imageSize = g_kinect.getDepthTexSize();
	imageBuffer = new unsigned char[imageSize];

	glutMainLoop();
}