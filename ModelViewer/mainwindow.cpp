#include "mainwindow.h"
#include "ui_mainwindow.h"

//#include <QtWidgets/QToolButton>
//#include <QtWidgets/QToolBar>
//#include <QtCore/QDir>
//#include <QtWidgets/QFileDialog>

#include <QtGui/qevent.h>

MainWindow::MainWindow(QWidget *parent) : AdriMainWindow(parent)
{
	ui->setupUi(this);

	ui->verticalLayout_4->removeWidget(ui->glCustomWidget);
	ui->glCustomWidget = new GLWidget(ui->frame);

	ui->glCustomWidget->setObjectName(QStringLiteral("glCustomWidget"));
	QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(ui->glCustomWidget->sizePolicy().hasHeightForWidth());
	ui->glCustomWidget->setSizePolicy(sizePolicy);
	ui->verticalLayout_4->addWidget(ui->glCustomWidget);
	widget = (GLWidget*)ui->glCustomWidget;

	widget->parent = this;

	
    // conexiones
    //connect(ui->GridDraw_boundary, SIGNAL(released()), ui->glCustomWidget, SLOT(updateGridRender()));

    // Actualizaciones del grid.
	//connect(ui->voxelization_btn, SIGNAL(released()), ui->glCustomWidget, SLOT(computeProcess()));
	connect(ui->voxelization_btn, SIGNAL(released()), this, SLOT(Compute()));
	connect(ui->smoothingPasses, SIGNAL(valueChanged(int)), this, SLOT(changeSmoothingPasses(int)));
	connect(ui->auxValueInt, SIGNAL(valueChanged(int)), this, SLOT(changeAuxValueInt(int)));

	//connect(ui->nextStep_button, SIGNAL(released()), ui->glCustomWidget, SLOT(nextProcessStep()));
	//connect(ui->allNextStep_button, SIGNAL(released()), ui->glCustomWidget, SLOT(allNextProcessSteps()));

    connect(ui->prop_function_updt, SIGNAL(released()), widget, SLOT(PropFunctionConf()));

    connect(ui->paintModel_btn, SIGNAL(released()), widget, SLOT(paintModelWithGrid()));
    connect(ui->metricUsedCheck, SIGNAL(released()), widget, SLOT(PropFunctionConf()));

    connect(ui->drawInfluences_check, SIGNAL(released()), widget, SLOT(showHCoordinatesSlot()));

    connect(ui->coordTab, SIGNAL(currentChanged(int)), widget, SLOT(active_GC_vs_HC(int)));

    connect(ui->glCustomWidget, SIGNAL(updateSceneView()), this, SLOT(updateSceneView()));
    connect(ui->outlinerView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectObject(QModelIndex)));

    connect(ui->segmentation_btn, SIGNAL(toggled(bool)), this, SLOT(toogleToShowSegmentation(bool)));

    connect(ui->exportWeights_btn, SIGNAL(released()), widget, SLOT(exportWeightsToMaya()));

    connect(ui->expansionSlider, SIGNAL(sliderReleased()), this, SLOT(changeExpansionSlider()));
    connect(ui->expansionSlider, SIGNAL(valueChanged(int)), this, SLOT(updateExpansionSlidervalue(int)));

	connect(ui->thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(updateThresholdSlidervalue(int)));
	connect(ui->threshold_enable, SIGNAL(toggled(bool)), this, SLOT(enableThreshold(bool)));
	connect(ui->threshold_enable_adaptative, SIGNAL(toggled(bool)), this, SLOT(enableAdaptativeThreshold(bool)));
	
    connect(ui->smoothPropagationSlider, SIGNAL(sliderReleased()), this, SLOT(changeSmoothSlider()));
    connect(ui->smoothPropagationSlider, SIGNAL(valueChanged(int)), this, SLOT(updateSmoothSlidervalue(int)));

	connect(ui->auxValueInt, SIGNAL(valueChanged(int)), this, SLOT(changeAuxValueInt(int)));

    connect(ui->glCustomWidget, SIGNAL(jointDataShow(float, int)), this , SLOT(jointDataUpdate(float,int)));

	connect(ui->ip_axisX, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));
	connect(ui->ip_axisY, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));
	connect(ui->ip_axisZ, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));

	connect(ui->paintModel, SIGNAL(clicked()), this, SLOT(updateModelColors()));
	connect(ui->paintPlane, SIGNAL(clicked()), this, SLOT(updateClipingPlaneColor()));

	connect(ui->drawPlaneCheck, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->PlaneOrientX, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->planeOrientY, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->planeOrientZ, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->PlaneDataCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeVisModeForPlane(int)));
	
	connect(ui->positionPlaneSlider, SIGNAL(sliderMoved(int)), this, SLOT(changeSelPointForPlane(int)));

	connectSignals();

}

void MainWindow::Compute()
{
	widget->computeProcess();
}

MainWindow::~MainWindow()
{
}

void MainWindow::UpdateScene()
{
	//widget->paintModelWithData();
}

void MainWindow::changeSmoothSlider()
{
    float valueAux = ui->smoothPropagationSlider->value();
    float value = valueAux/10.0;

    ui->smoothPropagationEdit->setText(QString("%1").arg(value));
    widget->changeSmoothPropagationDistanceRatio(value);
}

void MainWindow::changeSmoothingPasses(int value)
{
    float valueAux = ui->smoothingPasses->value();
    widget->changeSmoothingPasses(valueAux);
}

void MainWindow::changeAuxValueInt(int value)
{
    widget->valueAux = value;
	widget->paintModelWithData();
	widget->updateGridRender();
}

void MainWindow::updateThresholdSlidervalue(int value)
{
	ui->threshold_edit->setText(QString("%1").arg((float)value/100.0));

	if(ui->threshold_enable->isChecked())
		widget->setThreshold((float)value/100.0);
	else
		widget->setThreshold( -10);
}
void MainWindow::enableThreshold(bool toogle)
{
	int value = ui->thresholdSlider->value();

	if(ui->threshold_enable->isChecked())
		widget->setThreshold((float)value/100.0);
	else
		widget->setThreshold( -10);
}

void MainWindow::enableAdaptativeThreshold(bool toogle)
{
	int value = ui->thresholdSlider->value();

	if(!ui->threshold_enable_adaptative->isChecked())
		enableThreshold(ui->threshold_enable->isChecked());
	else
		widget->setThreshold(0);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    int current = 0;
    bool checked = false;
    switch(event->key())
    {
    case Qt::Key_Up:
        current = ui->SliceSelectorXZ->value();
        if(current + 5 <= ui->SliceSelectorXZ->maximum())
        {
            ui->SliceSelectorXZ->setValue(current+5);
            widget->ChangeSliceXZ(current+5);
        }
        break;
    case Qt::Key_Down:
        current = ui->SliceSelectorXZ->value();
        if(current - 5 >= ui->SliceSelectorXZ->minimum())
        {
            ui->SliceSelectorXZ->setValue(current-5);
            widget->ChangeSliceXZ(current-5);
        }
        break;
    case Qt::Key_Right:
        current = ui->SliceSelectorXY->value();
        if(current + 5 <= ui->SliceSelectorXY->maximum())
        {
            ui->SliceSelectorXY->setValue(current+5);
            widget->ChangeSliceXY(current+5);
        }
        break;
    case Qt::Key_Left:
        current = ui->SliceSelectorXY->value();
        if(current - 5 >= ui->SliceSelectorXY->minimum())
        {
            ui->SliceSelectorXY->setValue(current-5);
            widget->ChangeSliceXY(current-5);
        }
        break;

	// Defined in the parent.
    //case Qt::Key_Plus:     
    //case Qt::Key_Minus:
    //case Qt::Key_0:
    //case Qt::Key_1:
    //case Qt::Key_2:
    //case Qt::Key_3:
	//case Qt::Key_4:
	//case Qt::Key_5:
    //case Qt::Key_6:
    //case Qt::Key_7:
    //case Qt::Key_8:
    //case Qt::Key_9:
    default:
		AdriMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::updateClipingPlaneData()
{
	double sliderPos = (float)ui->positionPlaneSlider->value()/1000.0;
	int visCombo = ui->PlaneDataCombo->currentIndex();
	int selectedVertex = ui->dataSource->text().toInt();
	bool checked = ui->drawPlaneCheck->checkState() == Qt::Checked;
	ui->positionPlaneEdit->setText(QString("%1").arg(sliderPos, 3, 'g', 3));

	int orient = 0;
	if(ui->planeOrientY->isChecked())
		orient = 1;
	else if(ui->planeOrientZ->isChecked())
		orient = 2;

	widget->setPlaneData(checked, selectedVertex,visCombo, sliderPos, orient);
}

void MainWindow::updateClipingPlaneColor()
{
	widget->paintPlaneWithData(true);
}

void MainWindow::updateModelColors()
{
	widget->paintModelWithData();
}

void MainWindow::toogleToShowSegmentation(bool toogle)
{
	widget->toogleToShowSegmentation(toogle);
}

void MainWindow::changeExpansionSlider()
{
    float valueAux = ui->expansionSlider->value();
    float value = 0;
    if(valueAux <= 100)
        value = ((float)ui->expansionSlider->value())/100.0;
    else
    {
        value = (((valueAux-100)/100)*2)+1.0;
    }

    ui->expansionValueEdit->setText(QString("%1").arg(value));
    ((GLWidget*)ui->glCustomWidget)->changeExpansionFromSelectedJoint(value);
}


/*

#include "ui_mainwindow.h"



#include <QtWidgets/QTreeView>
#include <QtGui/QStandardItem>

#include <gui/treeitem.h>
#include <gui/treemodel.h>

#include <gui/outliner.h>
#include <DataStructures/modelo.h>

#include <utils/util.h>
#include <globalDefinitions.h>

#include "GLWidget.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    widget->parent = this;

    // conexiones
    connect(ui->action_importModel, SIGNAL(triggered()), this, SLOT(ImportNewModel()) );
    connect(ui->actionAction_openScene, SIGNAL(triggered()), this, SLOT(OpenNewScene()));

    connect(ui->import_cage_s, SIGNAL(triggered()), this, SLOT(ImportCages()) );
    connect(ui->import_distances, SIGNAL(triggered()), this, SLOT(ImportDistances()) );

    connect(ui->shadingModeSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(ShadingModeChange(int)) );

    connect(ui->colorLayersCheck, SIGNAL(toggled(bool)), this, SLOT(EnableColorLayer(bool)) );
    connect(ui->ColorLayerSeletion, SIGNAL(valueChanged(int)), this, SLOT(ChangeColorLayerValue(int)) );

    connect(ui->actionImportSegementation, SIGNAL(triggered()), this, SLOT(DoAction()) );

    //connect(ui->prop_function_updt, SIGNAL(released()), this, SLOT(ChangeSourceVertex()));

    connect(ui->DistancesVertSource, SIGNAL(valueChanged(int)), this, SLOT(distancesSourceValueChange(int)));

    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(CloseApplication()) );
    connect(ui->processGC,  SIGNAL(triggered()), widget, SLOT(processGreenCoordinates()) );
    connect(ui->processHC,  SIGNAL(triggered()), widget, SLOT(processHarmonicCoordinates()));
    connect(ui->processMVC, SIGNAL(triggered()), widget, SLOT(processMeanValueCoordinates()));
    connect(ui->processAll, SIGNAL(triggered()), widget, SLOT(processAllCoords()));
    connect(ui->deformedMeshCheck, SIGNAL(released()), widget, SLOT(showDeformedModelSlot()));

    connect(ui->cagesComboBox, SIGNAL(currentIndexChanged(int)), widget, SLOT(ChangeStillCage(int)));
    connect(ui->enableStillCage, SIGNAL(toggled(bool)), this, SLOT(enableStillCage(bool)));

    connect(ui->HCGridDraw, SIGNAL(released()), widget, SLOT(showHCoordinatesSlot()));
    connect(ui->GridDraw_boundary, SIGNAL(released()), widget, SLOT(updateGridRender()));

    // Actualizaciones del grid.
    connect(ui->GridDraw_interior, SIGNAL(released()), widget, SLOT(updateGridRender()));
    connect(ui->GridDraw_exterior, SIGNAL(released()), widget, SLOT(updateGridRender()));
    connect(ui->allGrid_button, SIGNAL(released()), widget, SLOT(updateGridRender()));
    connect(ui->gridSlices_button, SIGNAL(released()), widget, SLOT(updateGridRender()));
    connect(ui->SliceSelectorXY, SIGNAL(valueChanged(int)), widget, SLOT(ChangeSliceXY(int)));
    connect(ui->SliceSelectorXZ, SIGNAL(valueChanged(int)), widget, SLOT(ChangeSliceXZ(int)));

    connect(ui->voxelization_btn, SIGNAL(released()), widget, SLOT(computeProcess()));

	//connect(ui->nextStep_button, SIGNAL(released()), widget, SLOT(nextProcessStep()));
	//connect(ui->allNextStep_button, SIGNAL(released()), widget, SLOT(allNextProcessSteps()));

    connect(ui->prop_function_updt, SIGNAL(released()), widget, SLOT(PropFunctionConf()));

    connect(ui->paintModel_btn, SIGNAL(released()), widget, SLOT(paintModelWithGrid()));
    connect(ui->metricUsedCheck, SIGNAL(released()), widget, SLOT(PropFunctionConf()));

    connect(ui->drawInfluences_check, SIGNAL(released()), widget, SLOT(showHCoordinatesSlot()));

    connect(ui->coordTab, SIGNAL(currentChanged(int)), widget, SLOT(active_GC_vs_HC(int)));

    connect(widget, SIGNAL(updateSceneView()), this, SLOT(updateSceneView()));
    connect(ui->outlinerView, SIGNAL(clicked(QModelIndex)), this, SLOT(selectObject(QModelIndex)));

    connect(ui->actionMove, SIGNAL(triggered()), this, SLOT(toogleMoveTool()));
    connect(ui->actionSelection, SIGNAL(triggered()), this, SLOT(toogleSelectionTool()));
    connect(ui->actionRotate, SIGNAL(triggered()), this, SLOT(toogleRotateTool()));
    connect(ui->visibility_btn, SIGNAL(toggled(bool)), this, SLOT(toogleVisibility(bool)));

	connect(ui->actionDoTests, SIGNAL(triggered()), this, SLOT(LaunchTests()));
    connect(ui->DataVisualizationCombo, SIGNAL(currentIndexChanged(int)),this, SLOT(DataVisualizationChange(int)) );

    connect(ui->exportWeights_btn, SIGNAL(released()), widget, SLOT(exportWeightsToMaya()));

    connect(ui->expansionSlider, SIGNAL(sliderReleased()), this, SLOT(changeExpansionSlider()));
    connect(ui->expansionSlider, SIGNAL(valueChanged(int)), this, SLOT(updateExpansionSlidervalue(int)));

	connect(ui->thresholdSlider, SIGNAL(valueChanged(int)), this, SLOT(updateThresholdSlidervalue(int)));
	connect(ui->threshold_enable, SIGNAL(toggled(bool)), this, SLOT(enableThreshold(bool)));
	connect(ui->threshold_enable_adaptative, SIGNAL(toggled(bool)), this, SLOT(enableAdaptativeThreshold(bool)));
	

    connect(ui->smoothPropagationSlider, SIGNAL(sliderReleased()), this, SLOT(changeSmoothSlider()));
    connect(ui->smoothPropagationSlider, SIGNAL(valueChanged(int)), this, SLOT(updateSmoothSlidervalue(int)));

	connect(ui->smoothingPasses, SIGNAL(valueChanged(int)), this, SLOT(changeSmoothingPasses(int)));

	connect(ui->auxValueInt, SIGNAL(valueChanged(int)), this, SLOT(changeAuxValueInt(int)));

    connect(widget, SIGNAL(jointDataShow(float, int)), this , SLOT(jointDataUpdate(float,int)));

	connect(ui->ip_axisX, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));
	connect(ui->ip_axisY, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));
	connect(ui->ip_axisZ, SIGNAL(valueChanged(int)), this, SLOT(changeInteriorPointPosition()));

	connect(ui->paintModel, SIGNAL(clicked()), this, SLOT(updateModelColors()));
	connect(ui->paintPlane, SIGNAL(clicked()), this, SLOT(updateClipingPlaneColor()));

	connect(ui->drawPlaneCheck, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->PlaneOrientX, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->planeOrientY, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->planeOrientZ, SIGNAL(clicked()), this, SLOT(updateClipingPlaneData()));
	connect(ui->PlaneDataCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeVisModeForPlane(int)));
	
	connect(ui->dataSource, SIGNAL(valueChanged(int)), this, SLOT(distancesSourceValueChange(int)));
	connect(ui->positionPlaneSlider, SIGNAL(sliderMoved(int)), this, SLOT(changeSelPointForPlane(int)));
}


void MainWindow::changeSlider(int)
{

}

void MainWindow::ImportNewModel()
{
    QFileDialog inFileDialog(0, "Selecciona un fichero", widget->sPathGlobal, "*.off *.txt");
    inFileDialog.setFileMode(QFileDialog::ExistingFile);
    QStringList fileNames;
     if (inFileDialog.exec())
         fileNames = inFileDialog.selectedFiles();

    if(fileNames.size() == 0)
        return;

    QFileInfo sPathAux(fileNames[0]);
    QString aux = sPathAux.canonicalPath();
    QString sModelPath = aux;
    int ini = fileNames[0].indexOf("/",aux.length());
    aux = fileNames[0].right(fileNames[0].length()-ini);
    QString sModelPrefix = aux.left(aux.length()-4);

    widget->readModel(fileNames[0].toStdString(), sModelPrefix.toStdString(), sModelPath.toStdString());
}

void MainWindow::OpenNewScene()
{
    QFileDialog inFileDialog(0, "Selecciona un fichero", widget->sPathGlobal, "*.txt");
    inFileDialog.setFileMode(QFileDialog::ExistingFile);
    QStringList fileNames;
     if (inFileDialog.exec())
         fileNames = inFileDialog.selectedFiles();

    if(fileNames.size() == 0)
        return;

    QFileInfo sPathAux(fileNames[0]);
    QString aux = sPathAux.canonicalPath();
    QString sModelPath = aux;
    int ini = fileNames[0].indexOf("/",aux.length());
    aux = fileNames[0].right(fileNames[0].length()-ini);
    QString sModelPrefix = aux.left(aux.length()-4);

    widget->readScene(fileNames[0].toStdString(), sModelPrefix.toStdString(), sModelPath.toStdString());
}

void MainWindow::LaunchTests()
{
    QFileDialog inFileDialog(0, "Selecciona un fichero", widget->sPathGlobal, "*.txt");
    inFileDialog.setFileMode(QFileDialog::ExistingFile);
    QStringList fileNames;
     if (inFileDialog.exec())
         fileNames = inFileDialog.selectedFiles();

    if(fileNames.size() == 0)
        return;

    QFileInfo sPathAux(fileNames[0]);
    QString aux = sPathAux.canonicalPath();
    QString sModelPath = aux;
    int ini = fileNames[0].indexOf("/",aux.length());
    aux = fileNames[0].right(fileNames[0].length()-ini);
    QString sModelPrefix = aux.left(aux.length()-4);

	widget->doTests(fileNames[0].toStdString(), sModelPrefix.toStdString(), sModelPath.toStdString());
}

void MainWindow::updateSmoothSlidervalue(int)
{
    float valueAux = ui->smoothPropagationSlider->value();
    float value = valueAux/10.0;

    ui->smoothPropagationEdit->setText(QString("%1").arg(value));
}

void MainWindow::changeSmoothSlider()
{
    float valueAux = ui->smoothPropagationSlider->value();
    float value = valueAux/10.0;

    ui->smoothPropagationEdit->setText(QString("%1").arg(value));
    widget->changeSmoothPropagationDistanceRatio(value);
}

void MainWindow::changeSmoothingPasses(int)
{
    float valueAux = ui->smoothingPasses->value();
    widget->changeSmoothingPasses(valueAux);
}



void MainWindow::updateThresholdSlidervalue(int value)
{
	ui->threshold_edit->setText(QString("%1").arg((float)value/100.0));

	if(ui->threshold_enable->isChecked())
		widget->setThreshold((float)value/100.0);
	else
		widget->setThreshold( -10);
}
void MainWindow::enableThreshold(bool toogle)
{
	int value = ui->thresholdSlider->value();

	if(ui->threshold_enable->isChecked())
		widget->setThreshold((float)value/100.0);
	else
		widget->setThreshold( -10);
}

void MainWindow::enableAdaptativeThreshold(bool toogle)
{
	int value = ui->thresholdSlider->value();

	if(!ui->threshold_enable_adaptative->isChecked())
		enableThreshold(ui->threshold_enable->isChecked());
	else
		widget->setThreshold(0);
}

void MainWindow::updateExpansionSlidervalue(int)
{
    float valueAux = ui->expansionSlider->value();
    float value = 0;
    if(valueAux <= 100)
        value = ((float)ui->expansionSlider->value())/100.0;
    else
    {
        value = (((valueAux-100)/100)*9)+1.0;
    }

    ui->expansionValueEdit->setText(QString("%1").arg(value));
}

void MainWindow::changeExpansionSlider()
{
    float valueAux = ui->expansionSlider->value();
    float value = 0;
    if(valueAux <= 100)
        value = ((float)ui->expansionSlider->value())/100.0;
    else
    {
        value = (((valueAux-100)/100)*2)+1.0;
    }

    ui->expansionValueEdit->setText(QString("%1").arg(value));
    ((GLWidget*)widget)->changeExpansionFromSelectedJoint(value);
}

void MainWindow::changeVisModeForPlane(int)
{	
	updateClipingPlaneData();
}

void MainWindow::changeSelPointForPlane(int)
{
	updateClipingPlaneData();
}

void MainWindow::updateClipingPlaneData()
{
	double sliderPos = (float)ui->positionPlaneSlider->value()/1000.0;
	int visCombo = ui->PlaneDataCombo->currentIndex();
	int selectedVertex = ui->dataSource->text().toInt();
	bool checked = ui->drawPlaneCheck->checkState() == Qt::Checked;
	ui->positionPlaneEdit->setText(QString("%1").arg(sliderPos, 3, 'g', 3));

	int orient = 0;
	if(ui->planeOrientY->isChecked())
		orient = 1;
	else if(ui->planeOrientZ->isChecked())
		orient = 2;

	widget->setPlaneData(checked, selectedVertex,visCombo, sliderPos, orient);
}

void MainWindow::updateClipingPlaneColor()
{
	widget->paintPlaneWithData(true);
}

void MainWindow::updateModelColors()
{
	widget->paintModelWithData();
}

void MainWindow::changeInteriorPointPosition()
{
    float valueAuxX = ui->ip_axisX->value();
	float valueAuxY = ui->ip_axisY->value();
	float valueAuxZ = ui->ip_axisZ->value();

	ui->axisX_edit->setText(QString("%1").arg(valueAuxX));
	ui->axisY_edit->setText(QString("%1").arg(valueAuxY));
	ui->axisZ_edit->setText(QString("%1").arg(valueAuxZ));

	widget->interiorPoint = Vector3d(valueAuxX,valueAuxY,valueAuxZ);
	widget->setPlanePosition(valueAuxX,valueAuxY,valueAuxZ);
}

void MainWindow::jointDataUpdate(float fvalue, int id)
{
    if(fvalue <=1)
    {
		ui->expansionSlider->setValue((int)round(fvalue*100));
    }
    else
    {
        int value = ((int)round((fvalue-1)/9*100)+100);
        ui->expansionSlider->setValue(value);
    }

    ui->expansionValueEdit->setText(QString("%1").arg(fvalue));

    ui->DistancesVertSource->setValue(id);
    distancesSourceValueChange(id);
}

void MainWindow::ImportCages()
{
    
    //QFileDialog inFileDialog(0, "Selecciona un fichero", widget->sPathGlobal, "*.off , *.txt");
    //inFileDialog.setFileMode(QFileDialog::ExistingFile);
    //QStringList fileNames;
    // if (inFileDialog.exec())
    //     fileNames = inFileDialog.selectedFiles();

    //if(fileNames.size() == 0)
    //    return;

    //widget->readCage(fileNames[0], widget->m);
    

    printf("Function deprecated\n"); fflush(0);
}

void MainWindow::toogleMoveTool()
{
    if(toolSelected == T_MOVETOOL)
    {
        ui->actionMove->setChecked(true);
        return;
    }

    ui->actionRotate->setChecked(false);
    ui->actionSelection->setChecked(false);
    changeTool(T_MOVETOOL);
}

void MainWindow::toogleRotateTool()
{
    if(toolSelected == T_ROTATETOOL)
    {
       ui->actionRotate->setChecked(true);
        return;
    }

    ui->actionMove->setChecked(false);
    ui->actionSelection->setChecked(false);
    changeTool(T_ROTATETOOL);

}

void MainWindow::toogleSelectionTool()
{
    if(toolSelected == T_SELECTTOOL)
    {
        ui->actionSelection->setChecked(true);
        return;
    }

    ui->actionMove->setChecked(false);
    ui->actionRotate->setChecked(false);
    changeTool(T_SELECTTOOL);
}

void MainWindow::changeTool(toolmode newtool)
{
    switch(newtool)
    {
    case T_MOVETOOL:
        widget->setContextMode(CTX_MOVE);
        break;
    case T_SELECTTOOL:
        widget->setContextMode(CTX_SELECTION);
        break;
    case T_ROTATETOOL:
        widget->setContextMode(CTX_ROTATION);
        break;
    default:
        printf("Hay algún problema con la seleccion de contexto.\n"); fflush(0);
        widget->setContextMode(CTX_SELECTION);
        break;
    }

    printf("ha habido un cambio\n"); fflush(0);

}

void MainWindow::ShadingModeChange(int option)
{
    widget->cleanShadingVariables();
    if (option == 0)
    {
        widget->ShadingModeFlag = SH_MODE_SMOOTH;
        widget->cleanShadingVariables();
    }

    if (option == 1)
    {
        widget->ShadingModeFlag = SH_MODE_FLAT;
        widget->cleanShadingVariables();
    }

    if (option == 2)
    {
        widget->ShadingModeFlag = SH_MODE_SMOOTH;
        widget->toogleModelToXRay();
    }

    if (option == 3)
    {
        widget->ShadingModeFlag = SH_MODE_FLAT;
        widget->toogleModelToLines();
    }
}

void MainWindow::toogleVisibility(bool toogle)
{
   widget->toogleVisibility(toogle);
}

void MainWindow::DataVisualizationChange(int mode)
{
    widget->changeVisualizationMode(mode);
}

void MainWindow::toogleToShowSegmentation(bool toogle)
{
   widget->toogleToShowSegmentation(toogle);
}

void MainWindow::EnableColorLayer(bool _b)
{
    widget->colorLayers = _b;
        if (_b) widget->colorLayerIdx = ui->ColorLayerSeletion->value();
}

void MainWindow::ChangeColorLayerValue(int value)
{
    widget->colorLayerIdx = value;
}

void MainWindow::updateSceneView(TreeItem* treeitem, treeNode* treenode)
{
    for(unsigned int i = 0; i< treenode->childs.size(); i++)
    {
        QList<QVariant> columnData;
        columnData << QString(((treeNode*)treenode->childs[i])->sName.c_str());// << ((treeNode*)treenode->childs[i])->nodeId;

        TreeItem* elem = new TreeItem(columnData, treeitem);
        elem->sName = ((treeNode*)treenode->childs[i])->sName;
        elem->item_id = ((treeNode*)treenode->childs[i])->nodeId;
        elem->type = ((treeNode*)treenode->childs[i])->type;

        updateSceneView(elem,treenode->childs[i]);

        treeitem->appendChild(elem);
    }

}

void MainWindow::updateSceneView()
{
    //treeViewModel->clear(); // Limpiamos la lista.

    TreeModel* treeViewModel;
    treeViewModel = new TreeModel();
    ui->outlinerView->setModel(treeViewModel);

    treeNode* root = new treeNode();
    outliner outl(widget->escena);
    outl.getSceneTree(root);

    QList<TreeItem*> parents;
    parents << treeViewModel->rootItem;
    updateSceneView(parents.back(), root);

    emit ui->outlinerView->dataChanged(ui->outlinerView->rootIndex(), ui->outlinerView->rootIndex());
    repaint();
    // Hay que actualizar la vista para que aparezca todo.
}


void MainWindow::selectObject(QModelIndex idx)
{
    //TreeModel* model = (TreeModel*)ui->outlinerView->model();

    if (!idx.isValid()) return;

    TreeItem *item = static_cast<TreeItem*>(idx.internalPointer());

    //item->checked = !item->checked;
    //return item->checked;


    if(item->item_id == 0)
        return;

    // Listado de elementos seleccionados.
    vector<unsigned int > lst;
    lst.push_back(item->item_id);
    widget->selectElements(lst);
	//widget->updateGridVisualization();
	widget->paintModelWithData();

    printf("Selected: %s con id: %d\n", item->sName.c_str(), item->item_id); fflush(0);

    repaint();
}

void MainWindow::ImportDistances()
{
    QFileDialog inFileDialog(0, "Selecciona un fichero", widget->sPathGlobal, "*.txt");
    inFileDialog.setFileMode(QFileDialog::ExistingFile);
    QStringList fileNames;
     if (inFileDialog.exec())
         fileNames = inFileDialog.selectedFiles();

    if(fileNames.size() == 0)
        return;

    widget->readDistances(fileNames[0]);
}

void MainWindow::DoAction()
{
    QFileDialog inFileDialog(0, "Selecciona un fichero", widget->sPathGlobal, "*.txt");
    inFileDialog.setFileMode(QFileDialog::ExistingFile);
    QStringList fileNames;
    if (inFileDialog.exec())
        fileNames = inFileDialog.selectedFiles();

    if(fileNames.size() == 0)
        return;

    widget->importSegmentation(fileNames[0]);
}

void MainWindow::ChangeSourceVertex()
{
    int ind = ui->DistancesVertSource->text().toInt();
    widget->UpdateVertexSource(ind);
}

void MainWindow::distancesSourceValueChange(int ind)
{
    widget->UpdateVertexSource(ind);
}

void MainWindow::enableStillCage(bool state)
{
    ui->cagesComboBox->setEnabled(ui->enableStillCage->isEnabled());
    widget->stillCageAbled = ui->enableStillCage->isEnabled();

    if(ui->cagesComboBox->count()>0)
        widget->ChangeStillCage(ui->cagesComboBox->currentIndex());
}

void MainWindow::CloseApplication()
{
    exit(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}



*/