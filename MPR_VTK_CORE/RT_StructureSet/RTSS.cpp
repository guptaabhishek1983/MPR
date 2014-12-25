#include "RTSS.h"
#include "rad_logger.h"
#include "rad_template_helper.h"
#include "rad_util.h"

#include "vtkPoints.h"

#define __FILENAME__ "RTStructureSet.cpp"
#undef  __MODULENAME__
#define __MODULENAME__ "RTStructureSet"

using namespace RTViewer;
/*****************************************************************
----------------------- RT STRUCTURE SET STORAGE CLASS -------------------
*****************************************************************/


ROIContourSequence::ROIContourSequence()
{
	this->_contourPointsVTK = vtkPoints::New();
}
ROIContourSequence::ROIContourSequence(string referencedCTImageSOPUID)
{
	this->_referencedCTImageSOPUID = referencedCTImageSOPUID;
}
void ROIContourSequence::UpdateContourPoints(int idx, double x, double y, double z)
{
	this->_contourPoints.at(idx).x = x;
	this->_contourPoints.at(idx).y = y;
	this->_contourPoints.at(idx).z = z;
}
ROIContourSequence::~ROIContourSequence()
{

}



StructureSetROISequence::StructureSetROISequence()
{
	this->_selected = true;
}
StructureSetROISequence::~StructureSetROISequence()
{

}

ROIContourSequence* StructureSetROISequence::getROIContourSequence(string referencedCTSOPInstanceUID)
{
	for (int i = 0; i<_roiContourSequences.size(); i++)
	{
		if (strcmp(_roiContourSequences.at(i)->getReferencedCTImageSOPUID().c_str(), referencedCTSOPInstanceUID.c_str()) == 0)
			return _roiContourSequences.at(i);
	}
	return NULL;
}

vector<ROIContourSequence*> StructureSetROISequence::getROIContourSequenceVector(string referencedCTSOPInstanceUID)
{
	vector<ROIContourSequence*> out;
	for (int i = 0; i<_roiContourSequences.size(); i++)
	{
		if (strcmp(_roiContourSequences.at(i)->getReferencedCTImageSOPUID().c_str(), referencedCTSOPInstanceUID.c_str()) == 0)
			out.push_back(_roiContourSequences.at(i));
	}
	return out;
}

RTStructureSet::RTStructureSet()
{

}

RTStructureSet::RTStructureSet(RTDcmtkDicomInterface* rtStructureSetDicom)
{
	computed = false;
	

	DcmItem *structureSetROISequenceItem = NULL;
	DcmItem *referenceFrameOfReferenceItem = NULL;
	signed long structureSetROISequenceCount = 0;
	signed long referenceFrameOfReferenceCount = 0;
	this->studyUID = string(rtStructureSetDicom->Get_STUDY_INSTANCE_UID());
	this->sopUID = string(rtStructureSetDicom->Get_SOP_INSTANCE_UID());
	this->seriesUID = string(rtStructureSetDicom->Get_SERIES_INSTANCE_UID());
	// find and get referenced frame UID. 
	while (rtStructureSetDicom->dataset->findAndGetSequenceItem(DCM_ReferencedFrameOfReferenceSequence, referenceFrameOfReferenceItem,
		referenceFrameOfReferenceCount++).good())
	{
		const char * frameOfRefernceUID;
		referenceFrameOfReferenceItem->findAndGetString(DCM_FrameOfReferenceUID, frameOfRefernceUID);
		this->frameRefUID = string(frameOfRefernceUID);

		signed long referencedStudySequenceCount = 0;
		DcmItem *referencedStudySequenceItem = NULL;
		while (referenceFrameOfReferenceItem->findAndGetSequenceItem(DCM_RTReferencedStudySequence,
			referencedStudySequenceItem,
			referencedStudySequenceCount++).good())
		{
			signed long referencedSeriesSequenceCount = 0;
			DcmItem *referencedSeriesSequenceItem = NULL;
			while (referencedStudySequenceItem->findAndGetSequenceItem(DCM_RTReferencedSeriesSequence,
				referencedSeriesSequenceItem,
				referencedSeriesSequenceCount++).good())
			{
				const char * referencedSeriesUID;
				referencedSeriesSequenceItem->findAndGetString(DCM_SeriesInstanceUID, referencedSeriesUID);
				this->referencedCTSeriesUID = string(referencedSeriesUID);
			}
		}
	}

	while (rtStructureSetDicom->dataset->findAndGetSequenceItem(DCM_StructureSetROISequence, structureSetROISequenceItem,
		structureSetROISequenceCount++).good())
	{
		StructureSetROISequence* _roiSequence = new StructureSetROISequence();

		const char * structureSetROISequenceROINumber;
		const char * structureSetROISequenceROIName;
		structureSetROISequenceItem->findAndGetString(DCM_ROINumber, structureSetROISequenceROINumber);
		structureSetROISequenceItem->findAndGetString(DCM_ROIName, structureSetROISequenceROIName);

		//if (strcmp(structureSetROISequenceROIName, "BODY") == 0)
		{
			// Store Structure set ROIs based on ROI number.
			// This will help us in sequentially displaying ROIs on UI.
			_roiSequence->setName(structureSetROISequenceROIName);
			_roiSequence->setNumber(convert_to_int(structureSetROISequenceROINumber));
			_roiSequence->Reslicer = vtkRTSSMPR::New();

			this->structureSetROISequences.insert(make_pair(convert_to_int(structureSetROISequenceROINumber), _roiSequence));
		}
		//if (structureSetROISequenceCount > 1) break;
		//break;
	}

	int step = 100 / this->structureSetROISequences.size();

	DcmItem *structureSetROIContourSequenceItem = NULL;
	signed long structureSetROIContourCount = 0;
	while (rtStructureSetDicom->dataset->findAndGetSequenceItem(DCM_ROIContourSequence, structureSetROIContourSequenceItem,
		structureSetROIContourCount++).good())
	{
		// Fetch Referenced Structure Set ROI number.  
		const char * referencedStructureSetROINumber = NULL;
		structureSetROIContourSequenceItem->findAndGetString(DCM_ReferencedROINumber, referencedStructureSetROINumber);
		int _referencedStructureSetROINumber = convert_to_int(referencedStructureSetROINumber); // convert to int.

		if (this->structureSetROISequences.count(_referencedStructureSetROINumber)>0)
		{
			std::map<int, StructureSetROISequence*>::iterator it = this->structureSetROISequences.find(_referencedStructureSetROINumber);
			StructureSetROISequence* _temp = it->second;
			RAD_LOG_CRITICAL("Loading ROI number:" << _temp->getNumber()<<" name:"<<_temp->getName());

			OFString roiContourColor;
			structureSetROIContourSequenceItem->findAndGetOFStringArray(DCM_ROIDisplayColor, roiContourColor);
			if (roiContourColor.empty())
			{
				roiContourColor = "255\\138\\138"; // default color.
			}
			vector<string> _tokenizedROIContourColor;
			tokenize(roiContourColor.c_str(), _tokenizedROIContourColor, "\\");
			_temp->setColor(convert_to_int(_tokenizedROIContourColor.at(0).c_str()), convert_to_int(_tokenizedROIContourColor.at(1).c_str()),
				convert_to_int(_tokenizedROIContourColor.at(2).c_str())); // set color in StructureSetROISequence.

			signed long contourItemCount = 0;
			DcmItem *roiContourItem = NULL;



			while (structureSetROIContourSequenceItem->findAndGetSequenceItem(DCM_ContourSequence, roiContourItem, contourItemCount++).good())
			{
				DcmItem *contourImageSequenceItem = NULL;
				if (roiContourItem->findAndGetSequenceItem(DCM_ContourImageSequence, contourImageSequenceItem, 0).good())
				{
					const char * referencedSopInstanceUID = NULL;
					OFCondition status = contourImageSequenceItem->findAndGetString(DCM_ReferencedSOPInstanceUID, referencedSopInstanceUID, true);
					if (status == EC_Normal)
					{



						ROIContourSequence* _roiContourSequence = new ROIContourSequence(referencedSopInstanceUID);
						const char * numberOfPoints;
						const char * contourData;

						roiContourItem->findAndGetString(DCM_NumberOfContourPoints, numberOfPoints);
						roiContourItem->findAndGetString(DCM_ContourData, contourData);
						_roiContourSequence->_contourPointsVTK = vtkPoints::New();

						_roiContourSequence->setNumberOfContourPoints(convert_to_int(numberOfPoints));
						vector<string> _tokenizedContourPoints;
						tokenize(contourData, _tokenizedContourPoints, "\\");
						for (int i = 0; i<_tokenizedContourPoints.size(); i++)
						{
							ROIContourSequence::Contour3DPoint temp;
							temp.x = convert_to_double(_tokenizedContourPoints.at(i++).c_str());
							temp.y = convert_to_double(_tokenizedContourPoints.at(i++).c_str());
							temp.z = convert_to_double(_tokenizedContourPoints.at(i).c_str());
							//pContour->setContourPoints(temp);
							_roiContourSequence->AddContourPoint(temp);
							_roiContourSequence->_contourPointsVTK->InsertNextPoint(temp.x, temp.y, temp.z);
						}
						_temp->addROIContourSequence(_roiContourSequence);
						if (_roiContourSequence->_contourPointsVTK->GetNumberOfPoints() > 0)
						{
							_temp->Reslicer->AddContour(_roiContourSequence->_contourPointsVTK);
						}


					}
					else
					{
						RAD_LOG_CRITICAL("Refernced CT SOP Instance UID, not found for this contour sequence.");
					}
				}

			}


		}
		else
		{
		}


	}
	
}

RTStructureSet::~RTStructureSet(void)
{

}

void RTStructureSet::ComputeSlicers(double* origin, double* spacing, double* directionX, double* directionY, int* dim)
{
	
	/*double vtkOrigin[3];	double spacing[3];	double directionX[3];	double directionY[3];	int dim[3];
	for (int j = 0; j<3; j++)
	{
		vtkOrigin[j] = origin[j];
		spacing[j] = spacing[j];
		directionX[j] = directionX[j];
		directionY[j] = directionY[j];
		dim[j] = dim[j];
	}*/
	float step = 100.0 / this->structureSetROISequences.size();
	map<int, StructureSetROISequence*>::iterator it;
	int idx = 1;
	for (it = this->structureSetROISequences.begin(); it != this->structureSetROISequences.end(); ++it)
	{
		it->second->getReslicer()->SetImageGeometry(origin, spacing, dim, directionX, directionY);
		it->second->getReslicer()->Update();
		//RAD_LOG_CRITICAL("Computed Volume fo Contour#"<< idx<<" is:"<<it->second->getReslicer()->GetVolume());
		idx++;
	}
}

#include <vtkPlane.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTransform.h>

void RTStructureSet::ComputeROI(int axis, double pos, double firstImagePosition[3], double lastImagePosition[3], vector<vector<vector<ROIPlotPoints>>>& outPoints, vector<ROIColor>& outColors)
{
	vtkPlane* plane = vtkPlane::New();
	switch (axis)
	{
		case AxialAxis:
		{
			pos = pos + 1;
			plane->SetNormal(0, 0, 1); // set plane normal to X-Axis
			plane->SetOrigin(0, 0, pos);// set plane origin where Y and Z points are 0. Since plane is normal to X-Axis hence it cuts at specified index.
		}
			break;

		case CoronalAxis:
		{
			plane->SetNormal(0, 1, 0); // set plane normal to Y-Axis
			plane->SetOrigin(0, pos, 0); // set plane origin where X and Z points are 0. Since plane is normal to Y-Axis hence it cuts at specified index.
		}
			break;

		case SagittalAxis:
		{
			
			plane->SetNormal(1, 0, 0); // set plane normal to X-Axis
			plane->SetOrigin(pos, 0, 0);// set plane origin where Y and Z points are 0. Since plane is normal to X-Axis hence it cuts at specified index.
		}
			break;
	}

	map<int, StructureSetROISequence*> roiSequences = this->getStructureSetROISequences();
	map<int, StructureSetROISequence*>::iterator it;
	for (it = roiSequences.begin(); it != roiSequences.end(); ++it)
	{
		// Set ROI Color.
		ROIColor roiColor;
		roiColor.roiNumber = ((StructureSetROISequence*)it->second)->getNumber();
		roiColor.r = ((StructureSetROISequence*)it->second)->getRColor();
		roiColor.g = ((StructureSetROISequence*)it->second)->getGColor();
		roiColor.b = ((StructureSetROISequence*)it->second)->getBColor();
		outColors.push_back(roiColor);

		StructureSetROISequence* _temp = (StructureSetROISequence*)it->second;
		vtkSmartPointer< vtkPolyData > cutPoly = _temp->getReslicer()->GetCut(plane);

		//_temp->getReslicer()->GetInput()->Print(cerr);

		//vtkTransform* _tfm = vtkTransform::New();
		//_tfm->Identity();
		//_tfm->RotateX(orient[0]);
		//_tfm->RotateY(orient[1]);
		//_tfm->RotateZ(orient[2]);
		///*switch (axis)
		//{
		//	case AxialAxis:
		//		_tfm->RotateX(orient[0]);
		//		_tfm->RotateY(orient[1]);
		//		break;
		//	case SagittalAxis:
		//		break;
		//	case CoronalAxis:
		//		break;
		//}
		//*/
		//_tfm->Update();
		//vtkSmartPointer<vtkTransformPolyDataFilter> _tfmPolyData = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
		//_tfmPolyData->SetInputData(cutPoly);
		//_tfmPolyData->SetTransform(_tfm);
		//_tfmPolyData->Update();
		//cutPoly = _tfmPolyData->GetOutput();

		vtkCellArray* verts = cutPoly->GetLines();
		vtkPoints* pPoints = cutPoly->GetPoints();
		vtkIdType nPts = 0;
		vtkIdType *ptIds = verts->GetPointer();

		double v[3];
		
		int count = 0;
		vector<vector<ROIPlotPoints>> ROIOutPoints;
		ROIOutPoints.resize(verts->GetNumberOfCells());
		for (verts->InitTraversal(); verts->GetNextCell(nPts, ptIds);)
		{
			for (int j = 0; j < nPts; j++)
			{
				pPoints->GetPoint(ptIds[j], v);
				ROIPlotPoints p;
				p.roiNumber = it->first;
				p.x = v[0];
				p.y = v[1];
				p.z = v[2];

				if (axis == AxialAxis)
				{
					p.x = p.x - (firstImagePosition[0]); //x-offset for volume wrt 0,0,0 orging (image patient position tag)
					p.y = p.y - (lastImagePosition[1]);//z-offset for volume wrt 0,0,0 orging (image patient position tag)

					if (p.y > ((lastImagePosition[1] + firstImagePosition[1]) / 2)) // (546+51/2)+51 298
						p.y = lastImagePosition[1] + (p.y - firstImagePosition[1]);//y = -546.2 + (y+51.2);
					else
						p.y = firstImagePosition[1] - (lastImagePosition[1] - p.y);//y = -51.2 - (-546.2-y);
					//////
					
				}
				if (axis == CoronalAxis)
				{
					p.x = p.x - (firstImagePosition[0]); //x-offset for volume wrt 0,0,0 orging (image patient position tag)
					p.y = p.z - (lastImagePosition[2]);//z-offset for volume wrt 0,0,0 orging (image patient position tag)
					if (p.y > ((lastImagePosition[2] + firstImagePosition[2]) / 2)) // (546+51/2)+51 298
						p.y = lastImagePosition[2] + (p.y - firstImagePosition[2]);//y = -546.2 + (y+51.2);
					else
						p.y = firstImagePosition[2] - (lastImagePosition[2] - p.y);//y = -51.2 - (-546.2-y);
				}
				if (axis == SagittalAxis)
				{
					p.x = p.y - firstImagePosition[1]; //y-offset for volume wrt 0,0,0 orging (image patient position tag)
					p.y = p.z - lastImagePosition[2];//z-offset for volume wrt 0,0,0 orging (image patient position tag)
					if (p.y> ((lastImagePosition[2] + firstImagePosition[2]) / 2)) // (546+51/2)+51 298
						p.y = lastImagePosition[2] + (p.y - firstImagePosition[2]);//y = -546.2 + (y+51.2);
					else
						p.y = firstImagePosition[2] - (lastImagePosition[2] - p.y);//y = -51.2 - (-546.2-y);
					//p.x = p.y - firstImagePosition[1]; //y-offset for volume wrt 0,0,0 orging (image patient position tag)
					//p.y = p.z - lastImagePosition[2];//z-offset for volume wrt 0,0,0 orging (image patient position tag)
					//if (p.y> ((lastImagePosition[2] + firstImagePosition[2]) / 2)) // (546+51/2)+51 298
					//	p.y = lastImagePosition[2] + (p.y - firstImagePosition[2]);//y = -546.2 + (y+51.2);
					//else
					//	p.y = firstImagePosition[2] - (lastImagePosition[2] - p.y);//y = -51.2 - (-546.2-y);
				}
				ROIOutPoints.at(count).push_back(p);
			}
			count++;
		}
		outPoints.push_back(ROIOutPoints);
	}

}