#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <gui/AdriViewer.h>
#include <gui/manipulator.h>

#include <DataStructures/DataStructures.h>
#include <DataStructures/Scene.h>
#include <DataStructures/Object.h>
#include <DataStructures/skeleton.h>
#include <DataStructures/Modelo.h>
#include <DataStructures/grid3D.h>

#include "AppMgr.h"
#include <Computation/ComputationMgr.h>

#include <QtGui\qopenglshaderprogram.h>
#include <QtGui\qopenglbuffer.h>
#include <QtGui\qopenglvertexarrayobject.h>

#include <render/gridRender.h>
#include <render/clipingPlaneRender.h>
#include "mainwindow.h"

#include <gui/sktCreator.h>

//#include "gui\sktCreator.h"

using namespace std;

/*class MainWindow;
class Object;
class Cage;
class scene;
class skeleton;
class joint;
//class gridRenderer;
class Modelo;
class grid3d;*/

class GLWidget : public AdriViewer
{
	Q_OBJECT

public:    
	GLWidget(QWidget * parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags flags = 0);

    gridRenderer* currentProcessGrid;

    // CAGES
    bool processGreenCoordinates();
    bool processHarmonicCoordinates();
    bool processMeanValueCoordinates();
    void active_GC_vs_HC(int tab);
    bool processAllCoords();
    bool deformMesh();
    void loadSelectableVertex(Cage* cage /* MyMesh& cage*/);
    void showDeformedModelSlot();
    void showHCoordinatesSlot();
    void ChangeStillCage(int id);

    void updateGridVisualization();

    void cleanWeights(gridRenderer* grRend);
	void toogleToShowSegmentation(bool toogle);    

    void getStatisticsFromData(string fileName, string name, string path);
    void readDistances(QString fileName);

    void PropFunctionConf();
    void setPlaneData(bool drawPlane, int pointPos, int mode, float sliderPos, int orient);

    void setThreshold(double value);

    void nextProcessStep();
    void allNextProcessSteps();

    void drawModel();
    void drawWithDistances();
    void drawWithCages();

    void ChangeViewingMode(int);
	virtual void changeVisualizationMode(int);

	virtual void paintModelWithData();
	void paintPlaneWithData(bool compute = false);

	virtual void readScene(string fileName, string name, string path);
	
	// Read snakes for bean
	void readSnakes(string fileName, string name, string path);

	virtual void saveScene(string fileName, string name, string path, bool compactMode = false);

	// Skeleton creator tool
	sktCreator* sktCr;
	void finishRiggingTool();

	// Interface management
	Vector2f pressMouse;
	bool pressed;
	bool dragged;

	bool showBulgeInfo;

	virtual void mouseReleaseEvent(QMouseEvent* e);
	virtual void mousePressEvent(QMouseEvent* e);
	virtual void mouseMoveEvent(QMouseEvent* e);
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void keyReleaseEvent(QKeyEvent* e);

	bool X_ALT_modifier;

	int getSelection();

	// Manipulation
	manipulator ToolManip;

	// Shaders
	shaderIdx preferredType;
	shaderIdx preferredNormalType;
	QOpenGLShaderProgram* m_currentShader;
	vector<QOpenGLShaderProgram*> m_shaders;

	vector<shaderIdx> m_pShadersPile;

	QOpenGLBuffer m_vertexPositionBuffer;
	QOpenGLBuffer m_vertexColorBuffer;
	QOpenGLVertexArrayObject m_vao;

	virtual void traceRayToObject(Geometry* geom, Vector2i& pt , 
								  Vector3d& rayDir, vector<Vector3d>& intersecPoints, 
								  vector<int>& triangleIdx);

	virtual void getFirstMidPoint(Geometry* geom, Vector3d& rayDir, 
								  vector<Vector3d>& intersecPoints, 
								  vector<int>& triangleIdx, Vector3d& point);

	//Computation Management
	vector<ComputationMgr> compMgr;
	AppMgr *appMgr;

protected:
    //virtual void postSelection(const QPoint& point);
	virtual void draw();
	virtual void drawWithNames();

	virtual void draw2DGraphics();
	virtual void drawBulgeCurve(int idDeformer, int idGroup, 
								Vector2i Origin, Vector2i rectSize);

	virtual void drawBulgePressurePlane(int idDeformer, int idGroup, 
										Vector2i Origin,Vector2i rectSize);

	virtual void setShaderConfiguration(shaderIdx type);
	virtual void prepareShaderProgram();
	virtual void init();

	//virtual void paintEvent(QPaintEvent *event);
	//void drawOverpaint(QPainter *painter, QPaintEvent *event);

	virtual void prepareVertexBufferObject();
	virtual void updateColorBufferObject();
	virtual void updateVertexBufferObject();

	virtual void setContextMode(contextMode ctx);
	virtual void endContextMode(contextMode ctx);

	virtual void setToolCrtMode(int ctx);
	virtual void setTool(ToolType ctx);

public slots:

    // GENERAL
    void ChangeSliceXZ(int slice);
    void ChangeSliceXY(int slice);

	//Metodos especificos
	virtual void LaunchTests();
    virtual void doTests(string fileName, string name, string path);
	void eigenMultiplication(int elements);

	void computeProcess(); // All air computation from scratch
	void enableDeformationAfterComputing(); // Scene updating after computation
	void updateOutliner();
	
	void initBulges(AirRig* rig);

	void computeWeights();
	void updateComputations();

	// TOREMOVE
	void computeNodeMasiveOptimized(DefGraph& graph, Modelo& model, int defId2);

	virtual void selectElements(vector<unsigned int > lst);

	void BuildTetrahedralization();
    
	void exportWeightsToMaya();
    void UpdateVertexSource(int id);
    void importSegmentation(QString fileName);
    void updateColorLayersWithSegmentation(int maxIdx);
	
	virtual void setTwistParams(double ini, double fin, bool enable, bool smooth);
	virtual void setBulgeParams(bool enable);

	virtual void setGlobalSmoothPasses(int globalSmooth);
	virtual void setLocalSmoothPasses(int localSmooth);

    virtual void changeSmoothPropagationDistanceRatio(float smoothRatioValue);
	virtual void changeExpansionFromSelectedJoint(float expValue);

	virtual void setSceneScale(float scale)
	{
		escena->sceneScale = scale;
		ToolManip.size = scale;
		scene::drawingNodeStdSize = scale;
	}

	void pushShader(shaderIdx newShader);
	void popShader();

	void setAuxValue(int value);
};

#endif // GLWIDGET_H
