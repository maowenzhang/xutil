#include "Pch/SketchPch.h"
#include "SketchVCSDraggingData.h"

#include <Server/Base/Assert/Assert.h>

using namespace SKs::Constraint;

SketchVCSDraggingData::SketchVCSDraggingData()
	: m_startPt(),
	m_bodiesToDrag(),
	m_geomtriesToDrag()
{
}

SketchVCSDraggingData::~SketchVCSDraggingData()
{
}

void SketchVCSDraggingData::cleanup()
{
	// do nothing to m_startPt
	m_bodiesToDrag.clear();
	m_geomtriesToDrag.clear();
}

void SketchVCSDraggingData::resetData(const AcGePoint3d& startPt)
{
	m_startPt = startPt;
	m_bodiesToDrag.clear();
	m_geomtriesToDrag.clear();
}

VCSCollection& SketchVCSDraggingData::bodiesToDrag()
{
	return m_bodiesToDrag;
}

const AcGePoint3d& SketchVCSDraggingData::startPt() const
{
	return m_startPt;
}

bool SketchVCSDraggingData::IsDragSingleGeom() const
{
	if (m_geomtriesToDrag.size() == 1)
		return true;
	return false;
}

void SketchVCSDraggingData::addBodyToDrag(VCSRigidBody* pVCSBody)
{
	m_bodiesToDrag.add(pVCSBody);
}

void SketchVCSDraggingData::addGeomToDrag(SketchSolverGeometry* pGeom)
{
	NEUTRON_ASSERT(NULL != pGeom);
	if (NULL == pGeom)
		return;

	std::vector<SketchSolverGeometry*>::iterator iter
		 = std::find(m_geomtriesToDrag.begin(), m_geomtriesToDrag.end(), pGeom);
	if (iter != m_geomtriesToDrag.end())
		return;

	m_geomtriesToDrag.push_back(pGeom);
}

bool SketchVCSDraggingData::isDragGeom(SketchSolverGeometry* pGeom) const
{
	std::vector<SketchSolverGeometry*>::const_iterator iter 
		= std::find(m_geomtriesToDrag.begin(), m_geomtriesToDrag.end(), pGeom);
	if (iter != m_geomtriesToDrag.end())
		return true;
	return false;
}

std::vector<SketchSolverGeometry*>& SketchVCSDraggingData::geometriesToDrag()
{
	return m_geomtriesToDrag;
}

void SketchVCSDraggingData::dumpSolverData(Ns::IString& strData)
{
	strData += _DNGI("\n--- Dragging Data --- \n");
	int num = static_cast<int>(m_geomtriesToDrag.size());
	strData += (boost::wformat(_DNGI("Total:  %1%\n")) %num).str();

}