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
    void changeSmoothPropagationDistanceRatio(float smoothRatioValue);
	void changeSmoothingPasses(int value);

    void PropFunctionConf();
    void setPlaneData(bool drawPlane, int pointPos, int mode, float sliderPos, int orient);

    void setThreshold(double value);

    void nextProcessStep();
    void allNextProcessSteps();

    void getStatisticsFromData(string fileName, string name, string path);
    void readDistances(QString fileName);

    void toogleToShowSegmentation(bool toogle);
    void changeExpansionFromSelectedJoint(float expValue);

    void drawModel();
    void drawWithDistances();
    void drawWithCages();

    void ChangeViewingMode(int);
    //void updateGridRender();

	void paintPlaneWithData(bool compute = false);

	virtual void changeVisualizationMode(int);


protected:
    virtual void postSelection(const QPoint& point);

public slots:

    // GENERAL
    void ChangeSliceXZ(int slice);
    void ChangeSliceXY(int slice);

	//Metodos especificos
    virtual void doTests(string fileName, string name, string path);
    void computeProcess();
	void BuildTetrahedralization();
    void VoxelizeModel(Modelo *m, bool onlyBorders = true);
    void exportWeightsToMaya();
    void UpdateVertexSource(int id);
    void importSegmentation(QString fileName);
    void updateColorLayersWithSegmentation(int maxIdx);

};

#endif // GLWIDGET_H
