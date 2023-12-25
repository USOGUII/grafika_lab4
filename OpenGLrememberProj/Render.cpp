#include "Render.h"




#include <windows.h>

#include <GL\gl.h>
#include <GL\glu.h>
#include "GL\glext.h"

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "MyShaders.h"

#include "ObjLoader.h"
#include "GUItextRectangle.h"

#include "Texture.h"

GuiTextRectangle rec;

bool textureMode = true;
bool lightMode = true;
bool eidolonshot = true;


//небольшой дефайн для упрощения кода
#define POP glPopMatrix()
#define PUSH glPushMatrix()


ObjFile *model;

Texture texture1;
Texture sTex;
Texture rTex;
Texture tBox;

Shader s[10];  //массивчик для десяти шейдеров
Shader frac;
Shader cassini;




//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	virtual void SetUpCamera()
	{

		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//класс недоделан!
class WASDcamera :public CustomCamera
{
public:
		
	float camSpeed;

	WASDcamera()
	{
		camSpeed = 0.4;
		pos.setCoords(5, 5, 5);
		lookPoint.setCoords(0, 0, 0);
		normal.setCoords(0, 0, 1);
	}

	virtual void SetUpCamera()
	{

		if (OpenGL::isKeyPressed('W'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*camSpeed;
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}
		if (OpenGL::isKeyPressed('S'))
		{
			Vector3 forward = (lookPoint - pos).normolize()*(-camSpeed);
			pos = pos + forward;
			lookPoint = lookPoint + forward;
			
		}

		LookAt();
	}

} WASDcam;


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		Shader::DontUseShaders();
		bool f1 = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		bool f2 = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		bool f3 = glIsEnabled(GL_DEPTH_TEST);
		
		glDisable(GL_DEPTH_TEST);
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();

		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
				glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}
		/*
		if (f1)
			glEnable(GL_LIGHTING);
		if (f2)
			glEnable(GL_TEXTURE_2D);
		if (f3)
			glEnable(GL_DEPTH_TEST);
			*/
	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света



//старые координаты мыши
int mouseX = 0, mouseY = 0;




float offsetX = 0, offsetY = 0;
float zoom=1;
float Time = 0;
float Time2 = 0;
int tick_o = 0;
int tick_n = 0;

//обработчик движения мыши
void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}


	if (OpenGL::isKeyPressed(VK_LBUTTON))
	{
		offsetX -= 1.0*dx/ogl->getWidth()/zoom;
		offsetY += 1.0*dy/ogl->getHeight()/zoom;
	}


	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y,60,ogl->aspect);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

//обработчик вращения колеса  мыши
void mouseWheelEvent(OpenGL *ogl, int delta)
{


	float _tmpZ = delta*0.003;
	if (ogl->isKeyPressed('Z'))
		_tmpZ *= 10;
	zoom += 0.2*zoom*_tmpZ;


	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;
}

//обработчик нажатия кнопок клавиатуры
void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}	   

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}

	if (key == 'S')
	{
		frac.LoadShaderFromFile();
		frac.Compile();

		s[0].LoadShaderFromFile();
		s[0].Compile();

		cassini.LoadShaderFromFile();
		cassini.Compile();
	}

	if (key == 'Q')
		Time = 0;
	
	if (key == 'J') {
		eidolonshot = false;
	}
}

void keyUpEvent(OpenGL *ogl, int key)
{

}


void DrawQuad()
{
	double A[] = { 0,0 };
	double B[] = { 1,0 };
	double C[] = { 1,1 };
	double D[] = { 0,1 };
	glBegin(GL_QUADS);
	glColor3d(.5, 0, 0);
	glNormal3d(0, 0, 1);
	glTexCoord2d(0, 0);
	glVertex2dv(A);
	glTexCoord2d(1, 0);
	glVertex2dv(B);
	glTexCoord2d(1, 1);
	glVertex2dv(C);
	glTexCoord2d(0, 1);
	glVertex2dv(D);
	glEnd();
}


ObjFile objModel,monkey, sniper, tiny, enigma, aa, eidolon, platform, frosttree, rock, grass;

Texture monkeyTex;
Texture morphTex, sniperTex, tinyTex, enigmaTex, aaTex, eidolonTex, platformTex, frosttreeTex, rockTex, grassTex;

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{

	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	
	


	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	//ogl->mainCamera = &WASDcam;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	/*
	//texture1.loadTextureFromFile("textures\\texture.bmp");   загрузка текстуры из файла
	*/


	frac.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	frac.FshaderFileName = "shaders\\frac.frag"; //имя файла фрагментного шейдера
	frac.LoadShaderFromFile(); //загружаем шейдеры из файла
	frac.Compile(); //компилируем

	cassini.VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	cassini.FshaderFileName = "shaders\\cassini.frag"; //имя файла фрагментного шейдера
	cassini.LoadShaderFromFile(); //загружаем шейдеры из файла
	cassini.Compile(); //компилируем
	

	s[0].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[0].FshaderFileName = "shaders\\light.frag"; //имя файла фрагментного шейдера
	s[0].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[0].Compile(); //компилируем

	s[1].VshaderFileName = "shaders\\v.vert"; //имя файла вершинного шейдер
	s[1].FshaderFileName = "shaders\\textureShader.frag"; //имя файла фрагментного шейдера
	s[1].LoadShaderFromFile(); //загружаем шейдеры из файла
	s[1].Compile(); //компилируем

	

	 //так как гит игнорит модели *.obj файлы, так как они совпадают по расширению с объектными файлами, 
	 // создающимися во время компиляции, я переименовал модели в *.obj_m

	glActiveTexture(GL_TEXTURE0);
	loadModel("models\\morph.obj", &objModel);
	morphTex.loadTextureFromFile("textures//__morphling_base_color.bmp");
	morphTex.bindTexture();

	loadModel("models\\tiny.obj", &tiny);
	tinyTex.loadTextureFromFile("textures//__tiny_01_base_color.bmp");
	tinyTex.bindTexture();

	loadModel("models\\aa.obj", &aa);
	aaTex.loadTextureFromFile("textures//__ancient_apparition_base_color.bmp");
	aaTex.bindTexture();

	loadModel("models\\enigma.obj", &enigma);
	enigmaTex.loadTextureFromFile("textures//__enigma_base_color.bmp");
	enigmaTex.bindTexture();

	loadModel("models\\platform.obj", &platform);
	platformTex.loadTextureFromFile("textures//1660928151_35-celes-club-p-tekstura-gruntovoi-dorogi-krasivo-43.bmp");
	platformTex.bindTexture();

	loadModel("models\\eidolon.obj", &eidolon);
	eidolonTex.loadTextureFromFile("textures//eidelon_eidolon_color.bmp");
	eidolonTex.bindTexture();

	loadModel("models\\frosttree.obj", &frosttree);
	frosttreeTex.loadTextureFromFile("textures//tree_pine_frond_snow_00b_frostivus_color_psd_ce5309b9.bmp");
	frosttreeTex.bindTexture();

	loadModel("models\\rock.obj", &rock);
	rockTex.loadTextureFromFile("textures//riveredge_rock008a_color_psd_ef9087d7.bmp");
	rockTex.bindTexture();

	loadModel("models\\grass.obj", &grass);
	grassTex.loadTextureFromFile("textures//plants_journey_001_color_psd_23526154.bmp");
	grassTex.bindTexture();

	tick_n = GetTickCount();
	tick_o = tick_n;

	rec.setSize(300, 100);
	rec.setPosition(10, ogl->getHeight() - 100-10);
	rec.setText("T - вкл/выкл текстур\nL - вкл/выкл освещение\nF - Свет из камеры\nG - двигать свет по горизонтали\nG+ЛКМ двигать свет по вертекали",0,0,0);

	
}

double f(double p1, double p2, double p3, double p4, double t)
{
	return (1 - t) * (1 - t) * (1 - t) * p1 + 3 * t * (1 - t) * (1 - t) * p2 + 3 * t * t * (1 - t) * p3 + t * t * t * p4;
}

double ff(double p1, double p2, double t1, double t2, double t)
{
	double t3 = t * t * t;
	double t2t = t * t;
	return (2 * t3 - 3 * t2t + 1) * p1 + (t3 - 2 * t2t + t) * t1 + (-2 * t3 + 3 * t2t) * p2 + (t3 - t2t) * t2;
}

double angle = 1;
bool forward = true;
bool forward1 = true;
double trackpoint[3] = { 0, 0, 0 };




void Render(OpenGL *ogl)
{   
	
	tick_o = tick_n;
	tick_n = GetTickCount();
	Time += (tick_n - tick_o) / 1000.0;
	Time2 += (tick_n - tick_o) / 1000.0;
	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	*/

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (!lightMode)
		glEnable(GL_LIGHTING);

	//альфаналожение
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;

	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	//размер блика
	glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//===================================
	//Прогать тут  

	if (forward) {
		Time += Time/1000;
		if (Time >= 1) {
			Time = 1;
			forward = false;
		}
	}
	else {
		Time -= Time * 100000000000;
		if (Time <= 0) {
			Time = 0;
			forward = true;
		}
	}

	if (Time > 1)
		Time = 0;

	if (forward1) {
		Time2 += Time2 / 10000000;
		if (Time2 >= 1) {
			Time2 = 1;
			forward1 = false;
		}
	}
	else {
		Time2 -= Time2 / 10000000;
		if (Time2 <= 0) {
			Time2 = 0;
			forward1 = true;
		}
	}

	if (Time2 > 1)
		Time2 = 0;

		
	double S1[] = { 3.1,2.6, 1.3 };
	double S2[] = { 3.2,4.2, 0.7 };
	double S3[] = { 4.8,5, 1.5 };
	double S4[] = { 3.1,2.6, 1.3 };
	double S[3];

	trackpoint[0] = f(S1[0], S2[0], S3[0], S4[0], Time2);
	trackpoint[1] = f(S1[1], S2[1], S3[1], S4[1], Time2);
	trackpoint[2] = f(S1[2], S2[2], S3[2], S4[2], Time2);

	glPushMatrix();
	glColor3d(0.7, 0.75, 0.86);
	glBegin(GL_TRIANGLES);
	glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
	glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
	glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

	glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
	glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
	glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
	glEnd();

	glBegin(GL_QUADS);
	glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
	glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
	glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
	glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

	glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
	glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);
	glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
	glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);

	glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
	glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
	glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
	glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
	glEnd();
	glPopMatrix();

	if (1) {
		double S1[] = { 4,3.3, 2 };
		double S2[] = { 3,3.5, 1.8 };
		double S3[] = { 2.5,5.7, 1.3 };
		double S4[] = { 4,3.3, 2 };
		double S[3];

		trackpoint[0] = f(S1[0], S2[0], S3[0], S4[0], Time2);
		trackpoint[1] = f(S1[1], S2[1], S3[1], S4[1], Time2);
		trackpoint[2] = f(S1[2], S2[2], S3[2], S4[2], Time2);

		glPushMatrix();
		glColor3d(0.7, 0.75, 0.86);
		glBegin(GL_TRIANGLES);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glEnd();

		glBegin(GL_QUADS);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);

		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glEnd();
		glPopMatrix();
	}

	if (1) {
		double S1[] = { 4,3.3, 2 };
		double S2[] = { 5,3.5, 2.8 };
		double S3[] = { 1.5,5.7, 1.3 };
		double S4[] = { 4,3.3, 2 };
		double S[3];

		trackpoint[0] = f(S1[0], S2[0], S3[0], S4[0], Time2);
		trackpoint[1] = f(S1[1], S2[1], S3[1], S4[1], Time2);
		trackpoint[2] = f(S1[2], S2[2], S3[2], S4[2], Time2);

		glPushMatrix();
		glColor3d(0.7, 0.75, 0.86);
		glBegin(GL_TRIANGLES);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glEnd();

		glBegin(GL_QUADS);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);

		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glEnd();
		glPopMatrix();
	}

	if (1) {
		double S1[] = { 3.3, 2.8, 1.4 };
		double S2[] = { 2.1, 5.4, 1.3 };
		double S3[] = { 3,2.5, 1.8 };
		double S4[] = { 3.3, 2.8, 1.4 };
		double S[3];

		trackpoint[0] = f(S1[0], S2[0], S3[0], S4[0], Time2);
		trackpoint[1] = f(S1[1], S2[1], S3[1], S4[1], Time2);
		trackpoint[2] = f(S1[2], S2[2], S3[2], S4[2], Time2);


		glPushMatrix();
		glColor3d(0.7, 0.75, 0.86);
		glBegin(GL_TRIANGLES);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glEnd();

		glBegin(GL_QUADS);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);

		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glEnd();
		glPopMatrix();
	}

	if (1) {
		double S1[] = { 3.3, 2.8, 1.6 };
		double S2[] = { 2.1, 1.4, 1.3 };
		double S3[] = { 5, 2.5, 1.8 };
		double S4[] = { 3.5, 2.8, 1.2 };
		double S[3];

		trackpoint[0] = f(S1[0], S2[0], S3[0], S4[0], Time2);
		trackpoint[1] = f(S1[1], S2[1], S3[1], S4[1], Time2);
		trackpoint[2] = f(S1[2], S2[2], S3[2], S4[2], Time2);

		glPushMatrix();
		glColor3d(0.7, 0.75, 0.86);
		glBegin(GL_TRIANGLES);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glEnd();

		glBegin(GL_QUADS);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);

		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glEnd();
		glPopMatrix();
	}

	if (1) {
		double S1[] = { 3.3, 2.8, 1.6 };
		double S2[] = { 2.4, 1.9, 1.7 };
		double S3[] = { 5, 1.5, 1.9 };
		double S4[] = { 3.5, 3, 1 };
		double S[3];

		trackpoint[0] = f(S1[0], S2[0], S3[0], S4[0], Time2);
		trackpoint[1] = f(S1[1], S2[1], S3[1], S4[1], Time2);
		trackpoint[2] = f(S1[2], S2[2], S3[2], S4[2], Time2);

		glPushMatrix();
		glColor3d(0.7, 0.75, 0.86);
		glBegin(GL_TRIANGLES);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glEnd();

		glBegin(GL_QUADS);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);

		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glEnd();
		glPopMatrix();
	}

	if (1) {
		double S1[] = { 3.3, 2.8, 1.6 };
		double S2[] = { 4, 2, 1.7 };
		double S3[] = { 2, 1.9, 1.9 };
		double S4[] = { 3.5, 3, 1 };
		double S[3];

		trackpoint[0] = f(S1[0], S2[0], S3[0], S4[0], Time2);
		trackpoint[1] = f(S1[1], S2[1], S3[1], S4[1], Time2);
		trackpoint[2] = f(S1[2], S2[2], S3[2], S4[2], Time2);


		glPushMatrix();
		glColor3d(0.7, 0.75, 0.86);
		glBegin(GL_TRIANGLES);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glEnd();

		glBegin(GL_QUADS);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);

		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] - 0.07, trackpoint[1] - 0.025, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);

		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.01, trackpoint[1] + 0.02, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2] + 0.05);
		glVertex3d(trackpoint[0] + 0.075, trackpoint[1] + 0.015, trackpoint[2]);
		glEnd();
		glPopMatrix();
	}

		double P1[] = { 1,5.7, 0.8 };
		double P2[] = { 1,2, 0.8 };
		double P3[] = { 1,2, 0.8 };
		double P4[] = { 1,5.7, 0.8 };
		double P[3];

		trackpoint[0] = f(P1[0], P2[0], P3[0], P4[0], Time);
		trackpoint[1] = f(P1[1], P2[1], P3[1], P4[1], Time);
		trackpoint[2] = f(P1[2], P2[2], P3[2], P4[2], Time);

		glPushMatrix();
		glColor3d(0.2, 0, 0.5);
		glBegin(GL_TRIANGLES);
		glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
		glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

		glVertex3d(trackpoint[0] + 0.15, trackpoint[1] + 0.03, trackpoint[2] + 0.1);
		glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
		glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
		glEnd();

		glColor3d(0.2, 0, 0.5);
		glBegin(GL_QUADS);
		glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
		glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
		glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
		glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

		glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
		glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);

		glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
		glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
		glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
		glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
		glEnd();
		glPopMatrix();

		if (1) {
			double P1[] = { 2,5.7, 0.8 };
			double P2[] = { 2,2, 0.8 };
			double P3[] = { 2,2, 0.8 };
			double P4[] = { 2,5.7, 0.8 };
			double P[3];

			trackpoint[0] = f(P1[0], P2[0], P3[0], P4[0], Time);
			trackpoint[1] = f(P1[1], P2[1], P3[1], P4[1], Time);
			trackpoint[2] = f(P1[2], P2[2], P3[2], P4[2], Time);

			glPushMatrix();
			glColor3d(0.2, 0, 0.5);
			glBegin(GL_TRIANGLES);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glEnd();

			glColor3d(0.2, 0, 0.5);
			glBegin(GL_QUADS);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);

			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glEnd();
			glPopMatrix();
		}

		if (1) {
			double P1[] = { 3,5.7, 0.8 };
			double P2[] = { 3,2, 0.8 };
			double P3[] = { 3,2, 0.8 };
			double P4[] = { 3,5.7, 0.8 };
			double P[3];

			trackpoint[0] = f(P1[0], P2[0], P3[0], P4[0], Time);
			trackpoint[1] = f(P1[1], P2[1], P3[1], P4[1], Time);
			trackpoint[2] = f(P1[2], P2[2], P3[2], P4[2], Time);

			glPushMatrix();
			glColor3d(0.2, 0, 0.5);
			glBegin(GL_TRIANGLES);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glEnd();

			glColor3d(0.2, 0, 0.5);
			glBegin(GL_QUADS);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);

			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glEnd();
			glPopMatrix();
		}

		if (1) {
			double P1[] = { 4,5.7, 0.8 };
			double P2[] = { 4,2, 0.8 };
			double P3[] = { 4,2, 0.8 };
			double P4[] = { 4,5.7, 0.8 };
			double P[3];

			trackpoint[0] = f(P1[0], P2[0], P3[0], P4[0], Time);
			trackpoint[1] = f(P1[1], P2[1], P3[1], P4[1], Time);
			trackpoint[2] = f(P1[2], P2[2], P3[2], P4[2], Time);

			glPushMatrix();
			glColor3d(0.2, 0, 0.5);
			glBegin(GL_TRIANGLES);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glEnd();

			glColor3d(0.2, 0, 0.5);
			glBegin(GL_QUADS);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);

			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glEnd();
			glPopMatrix();
		}

		if (1) {
			double P1[] = { 5,5.7, 0.8 };
			double P2[] = { 5,2, 0.8 };
			double P3[] = { 5,2, 0.8 };
			double P4[] = { 5,5.7, 0.8 };
			double P[3];

			trackpoint[0] = f(P1[0], P2[0], P3[0], P4[0], Time);
			trackpoint[1] = f(P1[1], P2[1], P3[1], P4[1], Time);
			trackpoint[2] = f(P1[2], P2[2], P3[2], P4[2], Time);

			glPushMatrix();
			glColor3d(0.2, 0, 0.5);
			glBegin(GL_TRIANGLES);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glEnd();

			glColor3d(0.2, 0, 0.5);
			glBegin(GL_QUADS);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);

			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2] + 0.1);
			glVertex3d(trackpoint[0] - 0.14, trackpoint[1] - 0.05, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);

			glVertex3d(trackpoint[0] + 0.001, trackpoint[1] + 0.002, trackpoint[2]);
			glVertex3d(trackpoint[0] + 0.1, trackpoint[1] + 0.2, trackpoint[2] + 0.1);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2] + 0.1);
			glVertex3d((trackpoint[0] + 0.15), trackpoint[1] + 0.03, trackpoint[2]);
			glEnd();
			glPopMatrix();
		}





	s[0].UseShader();

	//передача параметров в шейдер.  Шаг один - ищем адрес uniform переменной по ее имени. 
	int location = glGetUniformLocationARB(s[0].program, "light_pos");
	//Шаг 2 - передаем ей значение
	glUniform3fARB(location, light.pos.X(), light.pos.Y(),light.pos.Z());

	location = glGetUniformLocationARB(s[0].program, "Ia");
	glUniform3fARB(location, 0.2, 0.2, 0.2);

	location = glGetUniformLocationARB(s[0].program, "Id");
	glUniform3fARB(location, 1.0, 1.0, 1.0);

	location = glGetUniformLocationARB(s[0].program, "Is");
	glUniform3fARB(location, .7, .7, .7);


	location = glGetUniformLocationARB(s[0].program, "ma");
	glUniform3fARB(location, 0.2, 0.2, 0.1);

	location = glGetUniformLocationARB(s[0].program, "md");
	glUniform3fARB(location, 0.4, 0.65, 0.5);

	location = glGetUniformLocationARB(s[0].program, "ms");
	glUniform4fARB(location, 0.9, 0.8, 0.3, 25.6);

	location = glGetUniformLocationARB(s[0].program, "camera");
	glUniform3fARB(location, camera.pos.X(), camera.pos.Y(), camera.pos.Z());

	//первый пистолет
	//objModel.DrawObj();


	Shader::DontUseShaders();
	
	//второй, без шейдеров
	//glPushMatrix();
	//	glTranslated(-5,5,0);
	//	glRotated(90, 1, 0, 0);
	//	glScaled(1, 1, 1);
	//	objModel.DrawObj();
	//glPopMatrix();



	//обезьяна

	s[1].UseShader();
	int l = glGetUniformLocationARB(s[1].program,"tex"); 
	glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
	glPushMatrix();
	glTranslated(5, 3, 0);
	glRotated(90, 1, 0, 0);
	glRotated(220, 0, 1, 0);
	morphTex.bindTexture();
	objModel.DrawObj();
	glPopMatrix();


	s[1].UseShader();
	l = glGetUniformLocationARB(s[1].program, "tex");
	glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
	glPushMatrix();
	glTranslated(1.25, 3, 0);
	glRotated(90, 1, 0, 0);
	glRotated(150, 0, 1, 0);
	tinyTex.bindTexture();
	tiny.DrawObj();
	glPopMatrix();
	
	s[1].UseShader();
	l = glGetUniformLocationARB(s[1].program, "tex");
	glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
	glPushMatrix();
	glTranslated(3.3, 3, 0);
	glRotated(90, 1, 0, 0);
	glRotated(180, 0, 1, 0);
	aaTex.bindTexture();
	aa.DrawObj();
	glPopMatrix();
	
	if (eidolonshot) {
		s[1].UseShader();
		l = glGetUniformLocationARB(s[1].program, "tex");
		glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
		glPushMatrix();
		glTranslated(3, 7.5, 0);
		glRotated(90, 1, 0, 0);
		enigmaTex.bindTexture();
		enigma.DrawObj();
		glPopMatrix();
	}

	for (int countme = 0; countme <= 4; countme++) {
		s[1].UseShader();
		l = glGetUniformLocationARB(s[1].program, "tex");
		glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
		glPushMatrix();
		glTranslated(1+countme, 6, 0);
		glRotated(90, 1, 0, 0);
		glScaled(0.8, 0.8, 0.8);
		eidolonTex.bindTexture();
		eidolon.DrawObj();
		glPopMatrix();
	}

	s[1].UseShader();
	l = glGetUniformLocationARB(s[1].program, "tex");
	glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
	glPushMatrix();
	glTranslated(3, 4, 0);
	glScaled(0.25, 0.3, 0.5);
	glRotated(90, 1, 0, 0);
	platformTex.bindTexture();
	platform.DrawObj();
	glPopMatrix();

	for (int countme = 0; countme <= 10; countme += 3.2) {
		for (int countme1 = 0; countme1 <= 7; countme1 += 6) {
			s[1].UseShader();
			l = glGetUniformLocationARB(s[1].program, "tex");
			glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
			glPushMatrix();
			glTranslated(countme1, countme+1.5, 0);
			glRotated(90, 1, 0, 0);
			frosttreeTex.bindTexture();
			frosttree.DrawObj();
			glPopMatrix();
		}
	}

	for (int countme = 0; countme <= 5; countme++) {
		for (int countme1 = 0; countme1 <= 10; countme1++) {
			s[1].UseShader();
			l = glGetUniformLocationARB(s[1].program, "tex");
			glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
			glPushMatrix();
			glTranslated(1 + countme, 0 + countme1, 0);
			glRotated(90, 1, 0, 0);
			glScaled(0.4, 0.4, 0.4);
			grassTex.bindTexture();
			grass.DrawObj();
			glPopMatrix();
		}
	}

	for (int countme = -0.5; countme <= 4.5; countme++) {
		for (int countme1 = 0; countme1 <= 10; countme1++) {
			s[1].UseShader();
			l = glGetUniformLocationARB(s[1].program, "tex");
			glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
			glPushMatrix();
			glTranslated(0.5 + countme, 0 + countme1, 0);
			glRotated(90, 1, 0, 0);
			glScaled(0.4, 0.4, 0.4);
			grassTex.bindTexture();
			grass.DrawObj();
			glPopMatrix();
		}
	}

	for (int countme = 0; countme <= 10; countme+= 3.2) {
		for (int countme1 = 0; countme1 <= 7; countme1+=6) {
			s[1].UseShader();
			l = glGetUniformLocationARB(s[1].program, "tex");
			glUniform1iARB(l, 0);     //так как когда мы загружали текстуру грузили на GL_TEXTURE0
			glPushMatrix();
			glTranslated(countme1, countme, 0);
			glRotated(90, 1, 0, 0);
			glScaled(0.5 + countme/10, 0.5+ countme / 10, 0.5+ countme / 10);
			rockTex.bindTexture();
			rock.DrawObj();
			glPopMatrix();
		}
	}
	//////Рисование фрактала

	
	/*
	{

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,1,0,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		frac.UseShader();

		int location = glGetUniformLocationARB(frac.program, "size");
		glUniform2fARB(location, (GLfloat)ogl->getWidth(), (GLfloat)ogl->getHeight());

		location = glGetUniformLocationARB(frac.program, "uOffset");
		glUniform2fARB(location, offsetX, offsetY);

		location = glGetUniformLocationARB(frac.program, "uZoom");
		glUniform1fARB(location, zoom);

		location = glGetUniformLocationARB(frac.program, "Time");
		glUniform1fARB(location, Time);

		DrawQuad();

	}
	*/
	
	
	//////Овал Кассини
	
	/*
	{

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,1,0,1,-1,1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		cassini.UseShader();

		int location = glGetUniformLocationARB(cassini.program, "size");
		glUniform2fARB(location, (GLfloat)ogl->getWidth(), (GLfloat)ogl->getHeight());


		location = glGetUniformLocationARB(cassini.program, "Time");
		glUniform1fARB(location, Time);

		DrawQuad();
	}

	*/

	
	
	

	
	Shader::DontUseShaders();

	
	
}   //конец тела функции


bool gui_init = false;

//рисует интерфейс, вызывется после обычного рендера
void RenderGUI(OpenGL *ogl)
{
	
	Shader::DontUseShaders();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	

	glActiveTexture(GL_TEXTURE0);
	rec.Draw();


		
	Shader::DontUseShaders(); 



	
}

void resizeEvent(OpenGL *ogl, int newW, int newH)
{
	rec.setPosition(10, newH - 100 - 10);
}

