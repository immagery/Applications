#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <gui/AdriViewer.h>

#include <DataStructures/DataStructures.h>
#include <DataStructures/Scene.h>
#include <DataStructures/Object.h>
#include <DataStructures/skeleton.h>
#include <DataStructures/Modelo.h>
#include <DataStructures/grid3D.h>

#include <render/gridRender.h>
#include <render/clipingPlaneRender.h>
#include "mainwindow.h"

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
    //void updateGridRender();

	void paintPlaneWithData(bool compute = false);

	virtual void changeVisualizationMode(int);
	virtual void paintModelWithData();

	virtual void readScene(string fileName, string name, string path);
	virtual void saveScene(string fileName, string name, string path);


protected:
    virtual void postSelection(const QPoint& point);
	virtual void draw();
	
public slots:

    // GENERAL
    void ChangeSliceXZ(int slice);
    void ChangeSliceXY(int slice);

	//Metodos especificos
    virtual void doTests(string fileName, string name, string path);
    void computeProcess();
	virtual void selectElements(vector<unsigned int > lst);
	void BuildTetrahedralization();
    void VoxelizeModel(Modelo *m, bool onlyBorders = true);
    void exportWeightsToMaya();
    void UpdateVertexSource(int id);
    void importSegmentation(QString fileName);
    void updateColorLayersWithSegmentation(int maxIdx);

	
	
	virtual void setTwistParams(double ini, double fin, bool enable);

	virtual void setGlobalSmoothPasses(int globalSmooth);
	virtual void setLocalSmoothPasses(int localSmooth);

    virtual void changeSmoothPropagationDistanceRatio(float smoothRatioValue);
	virtual void changeExpansionFromSelectedJoint(float expValue);

};

#endif // GLWIDGET_H
