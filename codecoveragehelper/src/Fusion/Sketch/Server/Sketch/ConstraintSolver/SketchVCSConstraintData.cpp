#include "Pch/SketchPch.h"
#include "SketchVCSConstraintData.h"
#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchSolverLine.h"
#include "SketchSolverCircArc.h"
#include "SketchVCSDraggingData.h"

#include <Server/Base/Assert/Assert.h>
#include <VCSAPI.h>
#include "../Objects/ISketchConstraint.h"
#include <Server/Geometry/Geometry3D/TangentHandle.h>
#include <Server/Geometry/Geometry3D/Point3D.h>

using namespace Ns::Comp;
using namespace SKs;
using namespace SKs::Constraint;

SketchVCSConstraintDataItem::SketchVCSConstraintDataItem()
	: m_geoms(),
	m_pConsHandle(NULL),
	m_dConsValue(0.0),
	m_eConsType(eNone),
	m_bTempAddedConstraint(false),
	m_bIsMinimumValueCons(false),
	m_bIsVariableCons(false),
	m_biasPt(),
	m_bIsRelationMeet(true),
	m_pPatternHandle(NULL)
{
}

SketchVCSConstraintDataItem::SketchVCSConstraintDataItem(const std::vector<SketchSolverGeometry*>& geoms, 
	VCSConHandle* pConsHandle,
	double dConsValue,
	SketchConstraintType eConsType,
	bool bIsVariableCons,
	bool bTempAddedConstraint)
	: m_geoms(geoms),
	m_pConsHandle(pConsHandle),
	m_dConsValue(dConsValue),
	m_eConsType(eConsType),
	m_bTempAddedConstraint(bTempAddedConstraint),
	m_bIsMinimumValueCons(false),
	m_bIsVariableCons(bIsVariableCons),
	m_biasPt(),
	m_bIsRelationMeet(true),
	m_pPatternHandle(NULL)
{
}

SketchVCSConstraintDataItem::SketchVCSConstraintDataItem(const SketchVCSConstraintDataItem& rhs)
{
	m_geoms = rhs.m_geoms;
	m_pConsHandle = rhs.m_pConsHandle;
	m_dConsValue = rhs.m_dConsValue;
	m_eConsType = rhs.m_eConsType;
	m_bTempAddedConstraint = rhs.m_bTempAddedConstraint;
	m_bIsMinimumValueCons = rhs.m_bIsMinimumValueCons;
	m_bIsVariableCons = rhs.m_bIsVariableCons;
	m_biasPt = rhs.m_biasPt;
	m_bIsRelationMeet = rhs.m_bIsRelationMeet;
	m_pPatternHandle = rhs.m_pPatternHandle;
}

SketchVCSConstraintDataItem& SketchVCSConstraintDataItem::operator=(const SketchVCSConstraintDataItem& rhs)
{
	m_geoms = rhs.m_geoms;
	m_pConsHandle = rhs.m_pConsHandle;
	m_dConsValue = rhs.m_dConsValue;
	m_eConsType = rhs.m_eConsType;
	m_bTempAddedConstraint = rhs.m_bTempAddedConstraint;
	m_bIsMinimumValueCons = rhs.m_bIsMinimumValueCons;
	m_bIsVariableCons = rhs.m_bIsVariableCons;
	m_biasPt = rhs.m_biasPt;
	m_bIsRelationMeet = rhs.m_bIsRelationMeet;
	m_pPatternHandle = rhs.m_pPatternHandle;
	return *this;
}

SketchVCSConstraintDataItem::~SketchVCSConstraintDataItem()
{
	m_geoms.clear();
	m_pConsHandle = NULL;
	// do nothing to m_eConsType
	// do nothing to m_bTryAddedConstraint
	// do nothing to m_bIsMinimumValueCons	
	// do nothing to m_bIsVariableCons
	m_pPatternHandle = NULL;
}

SketchSolverGeometry* SketchVCSConstraintDataItem::geom1() const
{
	if (!m_geoms.empty())
		return m_geoms[0];
	return NULL;
}

void SketchVCSConstraintDataItem::geom1(SketchSolverGeometry* pGeom)
{
	NEUTRON_ASSERT(!m_geoms.empty());
	if (!m_geoms.empty())
		m_geoms[0] = pGeom;
}

SketchSolverGeometry* SketchVCSConstraintDataItem::geom2() const
{
	if (m_geoms.size() >= 2)
		return m_geoms[1];
	return NULL;
}

void SketchVCSConstraintDataItem::geom2(SketchSolverGeometry* pGeom)
{
	NEUTRON_ASSERT(m_geoms.size() >= 2);
	if (m_geoms.size() >= 2)
		m_geoms[1] = pGeom;
}

const std::vector<SketchSolverGeometry*>& SketchVCSConstraintDataItem::geoms() const
{
	return m_geoms;
}

VCSConHandle* SketchVCSConstraintDataItem::consHandle()
{
	return m_pConsHandle;
}

void SketchVCSConstraintDataItem::consHandle(VCSConHandle* pHandle, bool isRelationMeet)
{
	m_pConsHandle = pHandle;
	m_bIsRelationMeet = isRelationMeet;
}

VCSPatternHandle* SketchVCSConstraintDataItem::patternHandle()
{
	return m_pPatternHandle;
}

void SketchVCSConstraintDataItem::patternHandle(VCSPatternHandle* pHandle)
{
	m_pPatternHandle = pHandle;
}

double SketchVCSConstraintDataItem::consValue() const
{
	return m_dConsValue;
}

void SketchVCSConstraintDataItem::consValue(double val)
{
	m_dConsValue = val;
}

void SketchVCSConstraintDataItem::biasPt(const AcGePoint3d& biasPt)
{
	m_biasPt = biasPt;
}

const AcGePoint3d& SketchVCSConstraintDataItem::biasPt() const
{
	return m_biasPt;
}

void SketchVCSConstraintDataItem::dumpSolverData(std::vector<Ns::IString>& strData)
{
	Ns::IString temp = _DNGI("A--");
	if (NULL != geom1())
		temp += geom1()->dumpInfo();

	temp += _DNGI("\tB--");
	if (NULL != geom2())
		temp += geom2()->dumpInfo();

	// Get type strings
	static std::map<VCSConType, Ns::IString> conTypeStrings;
	if (conTypeStrings.empty())
	{
		conTypeStrings[kVCSAngVecVec3d] = _DNGI("kVCSAngVecVec3d");
		conTypeStrings[kVCSMtPtPt3d] = _DNGI("kVCSMtPtPt3d");
		conTypeStrings[kVCSMtPtLn3d] = _DNGI("kVCSMtPtLn3d");
		conTypeStrings[kVCSMtPtCir3d] = _DNGI("kVCSMtPtCir3d");
		conTypeStrings[kVCSMtPtCur3d] = _DNGI("kVCSMtPtCur3d");
		conTypeStrings[kVCSMtPtEll3d] = _DNGI("kVCSMtPtEll3d");
		conTypeStrings[kVCSConCirCir3d] = _DNGI("kVCSAngVecVec3d");
		conTypeStrings[kVCSDistPtPt3d] = _DNGI("kVCSDistPtPt3d");
		conTypeStrings[kVCSDistPtLn3d] = _DNGI("kVCSDistPtLn3d");
		conTypeStrings[kVCSDistPtPl3d] = _DNGI("kVCSDistPtPl3d");
		conTypeStrings[kVCSDistLnLn3d] = _DNGI("kVCSDistLnLn3d");
		conTypeStrings[kVCSDistLnPl3d] = _DNGI("kVCSDistLnPl3d");
		conTypeStrings[kVCSTanCirCir3d] = _DNGI("kVCSTanCirCir3d");
		conTypeStrings[kVCSTanLnCir3d] = _DNGI("kVCSTanLnCir3d");
		conTypeStrings[kVCSTanLnCur3d] = _DNGI("kVCSTanLnCur3d");
		conTypeStrings[kVCSTanCurCur3d] = _DNGI("kVCSTanCurCur3d");
		conTypeStrings[kVCSTanCirCur3d] = _DNGI("kVCSTanCirCur3d");
		conTypeStrings[kVCSSymmLnLnLn3d] = _DNGI("kVCSSymmLnLnLn3d");
		conTypeStrings[kVCSSymmPtPtLn3d] = _DNGI("kVCSSymmPtPtLn3d");
		conTypeStrings[kVCSSymmCirCirLn3d] = _DNGI("kVCSSymmCirCirLn3d");
	}
	temp += _DNGI("\tConsType--");
	if (NULL == m_pConsHandle) // VCSPatternHandle
	{
		strData.push_back(temp);
		return;
	}

	std::map<VCSConType, Ns::IString>::iterator iter
		= conTypeStrings.find(m_pConsHandle->mType);
	if (iter == conTypeStrings.end())
	{
		NEUTRON_ASSERT(false);
		temp += _DNGI("Unknown");
	}
	else
		temp += iter->second;
	
	strData.push_back(temp);
}

bool SketchVCSConstraintDataItem::isTempAddedConstraint() const
{
	return m_bTempAddedConstraint;
}

bool SketchVCSConstraintDataItem::isVariableCons() const
{
	//NEUTRON_ASSERT(NULL != m_pConsHandle);
	if (NULL == m_pConsHandle) // polygon constraint
		return true;
	if (NULL != m_pConsHandle->variable())
		return true;
	if (consType() == eSymmetric)
		return true;
	
	return false;
}

bool SketchVCSConstraintDataItem::isMinimumValueCons() const
{
	return m_bIsMinimumValueCons;
}

void SketchVCSConstraintDataItem::isMinimumValueCons(bool val)
{
	m_bIsMinimumValueCons = val;
}

SketchConstraintType SketchVCSConstraintDataItem::consType() const
{
	return m_eConsType;
}

void SketchVCSConstraintDataItem::consType(SketchConstraintType val)
{
	m_eConsType = val;
}

bool SketchVCSConstraintDataItem::isContains(SketchSolverGeometry* pGeom1, SketchSolverGeometry* pGeom2) const
{
	if (m_geoms.end() == std::find(m_geoms.begin(), m_geoms.end(), pGeom1))
		return false;
	if (m_geoms.end() == std::find(m_geoms.begin(), m_geoms.end(), pGeom2))
		return false;
	return true;
}

bool SketchVCSConstraintDataItem::containsGeom(SketchSolverGeometry* pGeom) const
{
	if (m_geoms.end() == std::find(m_geoms.begin(), m_geoms.end(), pGeom))
		return false;
	return true;
}

bool SketchVCSConstraintDataItem::isRelationMeet() const
{
	if (NULL != m_pConsHandle)
	{
		double dist = m_pConsHandle->evaluate();
		if (fabs(dist) < 1.0e-6)
			return true;
		return false;
	}

	return m_bIsRelationMeet;
}

SketchVCSConstraintData::SketchVCSConstraintData()
	: m_solverGeomConsList(),
	m_pCachedConsItem(NULL),
	m_curConsItem(),
	m_bHasInvalidConstraint(false),
	m_bIsAddingNewConstraint(false),
	m_tempGroundedGeoms(),
	m_pNewAddConstraint(NULL)
{
}

SketchVCSConstraintData::SketchVCSConstraintData(SketchSolverGeometry* pGeom1, SketchSolverGeometry* pGeom2)
	: m_solverGeomConsList(),
	m_pCachedConsItem(NULL),
	m_curConsItem(),
	m_bHasInvalidConstraint(false),
	m_bIsAddingNewConstraint(false),
	m_tempGroundedGeoms(),
	m_pNewAddConstraint(NULL)
{
}

SketchVCSConstraintData::~SketchVCSConstraintData()
{
	cleanup();
}

SketchSolverGeometry* SketchVCSConstraintData::geom1() const
{
	return m_curConsItem.geom1();
}

void SketchVCSConstraintData::geom1(SketchSolverGeometry* pGeom)
{
	m_curConsItem.geom1(pGeom);
}

SketchSolverGeometry* SketchVCSConstraintData::geom2() const
{
	return m_curConsItem.geom2();
}

void SketchVCSConstraintData::geom2(SketchSolverGeometry* pGeom)
{
	m_curConsItem.geom2(pGeom);
}

double SketchVCSConstraintData::consValue() const
{
	return m_curConsItem.consValue();
}

void SketchVCSConstraintData::consValue(double dVal)
{
	m_curConsItem.consValue(dVal);
}

bool SketchVCSConstraintData::hasInvalidConstraint() const
{
	return m_bHasInvalidConstraint;
}

void SketchVCSConstraintData::hasInvalidConstraint(bool val)
{
	// current doesn't support handling of invalid constraint (zhanglo)
	NEUTRON_ASSERT(false);
	m_bHasInvalidConstraint = val;
}

bool SketchVCSConstraintData::isAddingNewConstraint() const
{
	return m_bIsAddingNewConstraint;
}

void SketchVCSConstraintData::isAddingNewConstraint(bool val)
{
	m_bIsAddingNewConstraint = val;
}

SketchConstraintType SketchVCSConstraintData::consType() const
{
	return m_curConsItem.consType();
}

void SketchVCSConstraintData::newAddedConsHandle(std::vector<VCSConHandle*>& newAddedCons)
{
	if (m_solverGeomConsList.empty())
		return;

	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (eInternalPointOnVariableCurve == pItem->consType())
			continue;
		newAddedCons.push_back(pItem->consHandle());
	}
}

void SketchVCSConstraintData::getUnmeetConsHandle(SketchToVCSConstraintMap& unmeetCons)
{
	if (m_sketchToVCSConsMap.empty())
		return;

	BOOST_FOREACH(SketchToVCSConstraintMap::value_type& val, m_sketchToVCSConsMap)
	{
		SolverGeomConsList& itemList = val.second;

		bool bUnmeetCons = false;
		BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, itemList)
		{
			if (!pItem->isRelationMeet())
			{
				unmeetCons.insert(val);
				break;
			}
		}
	}
}

void SketchVCSConstraintData::addConsHandle(VCSConHandle* pConsHandle, bool isRelationMeet)
{
	NEUTRON_ASSERT(NULL != pConsHandle);
	m_curConsItem.consHandle(pConsHandle, isRelationMeet);

	SketchVCSConstraintDataItem* pItem = new SketchVCSConstraintDataItem(m_curConsItem);
	m_solverGeomConsList.push_back(pItem);
}

void SketchVCSConstraintData::addPatternHandle(VCSPatternHandle* pHandle)
{
	m_curConsItem.patternHandle(pHandle);

	SketchVCSConstraintDataItem* pItem = new SketchVCSConstraintDataItem(m_curConsItem);
	m_solverGeomConsList.push_back(pItem);
}

int SketchVCSConstraintData::geomCount()
{
	int iCount = 0;
	if (NULL != geom1())
		iCount++;
	if (NULL != geom2())
		iCount++;

	return iCount;
}

bool SketchVCSConstraintData::hasGeom1() const
{
	NEUTRON_ASSERT(NULL != geom1());
	if (NULL == geom1())
		return false;
	return true;
}

bool SketchVCSConstraintData::hasGeom1And2() const
{
	NEUTRON_ASSERT(NULL != geom1() && NULL != geom2());
	if (NULL == geom1() || NULL == geom2())
		return false;
	
	// If have two geometries to add constraint, shouldn't be the same geometry, need to fix if happens. (zhanglo)
	NEUTRON_ASSERT(geom1() != geom2());
	if (geom1() == geom2())
		return false;
	return true;
}

bool SketchVCSConstraintData::checkGeomTypes(ESolverGeometryType geomType)
{
	if (!hasGeom1And2())
		return false;

	if (geomType == geom1()->geomType() && geomType == geom2()->geomType())
		return true;

	return false;
}

bool SketchVCSConstraintData::checkGeomTypes(ESolverGeometryType geom1Type, ESolverGeometryType geom2Type)
{
	if (!hasGeom1And2())
		return false;

	if (geom1Type == geom1()->geomType() && geom2Type == geom2()->geomType())
		return true;

	if (geom1Type == geom2()->geomType() && geom2Type == geom1()->geomType())
	{
		// Swap them to make sure output is right
		SketchSolverGeometry* pGeomTemp = geom1();
		geom1(geom2());
		geom2(pGeomTemp);
		return true;
	}
	return false;
}

bool SketchVCSConstraintData::checkGeomTypesAll(ESolverGeometryType geomType)
{
	BOOST_FOREACH(SketchSolverGeometry* item, m_curConsItem.geoms())
	{
		if (geomType != item->geomType())
			return false;
	}
	return true;
}

void SketchVCSConstraintData::resetData(const std::vector<SketchSolverGeometry*>& geoms,
	double dConsValue, 
	SketchConstraintType eConsType,
	bool bIsVariableCons,
	bool bTempAddConstraint)
{
	// Cache data
	if (NULL == m_pCachedConsItem)
		m_pCachedConsItem = new SketchVCSConstraintDataItem(m_curConsItem);
	else
		*m_pCachedConsItem = m_curConsItem;

	m_curConsItem = SketchVCSConstraintDataItem(geoms, NULL, dConsValue, eConsType, bIsVariableCons, bTempAddConstraint);
}

void SketchVCSConstraintData::resetData(SketchSolverGeometry* pGeom1, 
	SketchSolverGeometry* pGeom2,
	double dConsValue, 
	SketchConstraintType eConsType,
	bool bIsVariableCons,
	bool bTempAddConstraint)
{
	std::vector<SketchSolverGeometry*> geoms;
	geoms.push_back(pGeom1);
	geoms.push_back(pGeom2);
	resetData(geoms, dConsValue, eConsType, bIsVariableCons, bTempAddConstraint);
}

void SketchVCSConstraintData::resetDataEX(SketchSolverGeometry* pGeom1, 
	SketchSolverGeometry* pGeom2, 
	double dConsValue,
	bool bIsVariableCons)
{
	// Use the old constraint type
	std::vector<SketchSolverGeometry*> geoms;
	geoms.push_back(pGeom1);
	geoms.push_back(pGeom2);
	resetData(geoms, dConsValue, m_curConsItem.consType(), bIsVariableCons);
}

void SketchVCSConstraintData::cleanup()
{
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		delete pItem;
		pItem = NULL;
	}
	m_solverGeomConsList.clear();

	if (NULL != m_pCachedConsItem)
	{
		delete m_pCachedConsItem;
		m_pCachedConsItem = NULL;
	}
	m_bHasInvalidConstraint = false;
	m_sketchToVCSConsMap.clear();
	m_bIsAddingNewConstraint = false;
	m_tempGroundedGeoms.clear();
	m_pNewAddConstraint = NULL;
}

int SketchVCSConstraintData::relatedConstraintNum(SketchSolverGeometry* pSolverGeom)
{
	int num = 0;
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		// Skip variable constraint (equal, symmetry, midpoint, extra)
		// (related with: add end points to drag together when drag tangent handle which has mid point constraint)
		if (pItem->isVariableCons())
			continue;
		const std::vector<SketchSolverGeometry*>& geoms = pItem->geoms();
		if (geoms.end() != std::find(geoms.begin(), geoms.end(), pSolverGeom))
			num++;
	}
	return num;
}

void SketchVCSConstraintData::dumpSolverData(std::vector<Ns::IString>& strData)
{
	Ns::IString temp = _DNGI("--- Constraint Data ---");
	int consNum = static_cast<int>(m_solverGeomConsList.size());
	temp += (boost::wformat(_DNGI("Total:  %1%\n")) %consNum).str();
	strData.push_back(temp);

	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		pItem->dumpSolverData(strData);
	}
}

void SketchVCSConstraintData::restoreData()
{
	NEUTRON_ASSERT(NULL != m_pCachedConsItem);
	if (NULL == m_pCachedConsItem)
		return;
	m_curConsItem = *m_pCachedConsItem;
}

void SketchVCSConstraintData::revertTempChanges(SketchConstraintSolverContext& ctx)
{
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (pItem->isTempAddedConstraint())
		{
			VCSConHandle* pHandle = pItem->consHandle();
			ctx.vcsInterface()->vcsSystem()->deleteConstraint(pHandle);
		}
	}
}

SketchVCSConstraintDataItem& SketchVCSConstraintData::curConsItem()
{
	return m_curConsItem;
}

bool SketchVCSConstraintData::getRelatedLinesFromPoint(SketchSolverGeometry* pPoint, std::vector<SketchSolverLine*>& relatedLines)
{
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (pItem->consType() != SKs::eLineMidPoint 
			&& pItem->consType() != SKs::eCoincident)
			continue;

		if (pItem->geom1() != pPoint && pItem->geom2() != pPoint)
			continue;

		SketchSolverGeometry* pOtherGeom = pItem->geom2();
		if (pItem->geom2() == pPoint)
			pOtherGeom = pItem->geom1();
		if (NULL == pOtherGeom || pOtherGeom->geomType() != eSolverGeomLine)
			continue;

		SketchSolverLine* pLine = pOtherGeom->solverGeom<SketchSolverLine>(eSolverGeomLine);
		if (NULL == pLine)
			continue;
		
		// Skip end points of line
		if (pLine->solverGeomEndPt() == pPoint || pLine->solverGeomStartPt() == pPoint)
			continue;

		// Add line which is the point located
		relatedLines.push_back(pLine);
	}
	return !relatedLines.empty();
}

bool SketchVCSConstraintData::hasRelatedConsWithLinePreventFixEndPoint(SketchSolverGeometry* pLine)
{
	// Check each constraint Item in the list
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (!pItem->containsGeom(pLine))
			continue;

		// Not fix if line's tangent with circle/others (or couldn't move if circle is somehow fixed)
		if (eHorizontal == pItem->consType()
			|| eVertical == pItem->consType()
			|| ePerpendicular == pItem->consType()
			|| eParallel == pItem->consType()
			|| eTangent == pItem->consType()
			|| eSmooth == pItem->consType())
		{
			return true;
		}

		if (eSymmetric == pItem->consType())
		{
			// Shouldn't fix the end points of symmetry lines, able to fix end point of symmetry base line
			// If we fix the end point, then the line's direction is fixed as the "symmetry point" on the line is fixed.			
			if (pItem->geoms()[2] != pLine)
				return true;
		}
	}
	return false;
}

bool SketchVCSConstraintData::shouldFixCircleArcEndPoint(SketchSolverGeometry* pCirArc, SketchSolverGeometry* pEndPt)
{
	int iConsOnEndPt = relatedConstraintNum(pEndPt);
	if (iConsOnEndPt > 2) // one is on circle, one is on-plane
		return false;

	// Check each constraint Item in the list
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (!pItem->containsGeom(pCirArc))
			continue;

		if (eTangent == pItem->consType()
			|| eSmooth == pItem->consType())
		{
			return false;
		}
	}
	return true;
}

bool SketchVCSConstraintData::hasRelatedConsWithCircleToMakeVariable(SketchSolverGeometry* pCircle, bool bDragCenterPoint)
{
	// Check each constraint Item in the list
	int iNumOfConsOnCircumFerence = 0;
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (pItem->geom1() != pCircle && pItem->geom2() != pCircle)
			continue;

		if (eTangent == pItem->consType()
			|| eCoincident == pItem->consType()
			|| ePerpendicular == pItem->consType())
		{
			iNumOfConsOnCircumFerence++;
			// Only need one constraint if current is dragging center point
			if (bDragCenterPoint)
				return true;
			if (iNumOfConsOnCircumFerence == 2)
				return true;
		}
		if (eSymmetric == pItem->consType())
		{
			if (bDragCenterPoint)
				return false;
			return true; // variable if drag circumference
		}
	}
	return false;
}

bool SketchVCSConstraintData::hasRelatedConsWithCircleToMakeVariableInSolving(SketchSolverGeometry* pCircle)
{
	// Check each constraint Item in the list
	int iNumOfConsOnCircumFerence = 0;
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (pItem->geom1() != pCircle && pItem->geom2() != pCircle)
			continue;

		if (eTangent == pItem->consType())
			return true;
		if (eCoincident == pItem->consType()
			|| ePerpendicular == pItem->consType())
		{
			iNumOfConsOnCircumFerence++;
			if (iNumOfConsOnCircumFerence == 2)
				return true;
		}
		if (eSymmetric == pItem->consType()
			|| eEqual == pItem->consType()
			|| eRadialDim == pItem->consType()
			|| eDiameterDim == pItem->consType()			
			|| eEllipseMajorRadiusDim == pItem->consType()
			|| eEllipseMinorRadiusDim == pItem->consType())
		{
			return true;
		}
	}
	return false;
}

bool SketchVCSConstraintData::hasRelatedConsBetweenGeometry(SketchSolverGeometry* pGeom1, 
	SketchSolverGeometry* pGeom2, 
	bool bIsExcludeVarCons)
{
	// Check each constraint Item in the list
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (pItem->isContains(pGeom1, pGeom2))
		{
			if (bIsExcludeVarCons && pItem->isVariableCons())
				continue;
			return true;
		}
	}
	return false;
}

bool SketchVCSConstraintData::handleSymmetryConsWhenDragging(SketchSolverGeometry* pGeom, bool isDragRelatedItem)
{
	// Check each constraint Item in the list
	std::vector<SketchSolverLine*> linesToFixLength;

	bool bHandled = false;
	// TODO: need to build a graph for check related constraint types on geometry
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (eSymmetric != pItem->consType())
			continue;
		
		// symmetry line
		SketchSolverGeometry* pGeom1 = pItem->geom1();
		SketchSolverGeometry* pGeom2 = pItem->geom2();
		SketchSolverGeometry* pGeomBase = pItem->geoms()[2];

		if (pGeom1->geomType() == eSolverGeomLine && pGeom2->geomType() == eSolverGeomLine)
		{
			SketchSolverLine* pLine1 = pGeom1->solverGeom<SketchSolverLine>(eSolverGeomLine);
			SketchSolverLine* pLine2 = pGeom2->solverGeom<SketchSolverLine>(eSolverGeomLine);
			if (pGeom == pGeomBase)
			{				
				pLine1->solverGeomStartPt()->setGrounded(true);
				pLine2->solverGeomStartPt()->setGrounded(true);
				bHandled = true;
			}
			else if (pGeom == pLine1|| pGeom == pLine2)
			{
				pGeomBase->setGrounded(true);

				// If drag line's end point, ground another line's end point
				if (isDragRelatedItem)
				{
					if (pGeom == pLine1)
						pLine2->solverGeomEndPt()->setGrounded(true);
					else
						pLine1->solverGeomEndPt()->setGrounded(true);
				}
				else
				{
					// If drag line itself, fix another line's length
					if (pGeom == pLine1)
						linesToFixLength.push_back(pLine2);
					else
						linesToFixLength.push_back(pLine1);
				}
				bHandled = true;
			}
		}
		// For point-point
		else if (pGeom1->geomType() == eSolverGeomPoint && pGeom2->geomType() == eSolverGeomPoint)
		{
			if (pGeom == pGeomBase)
			{
				pickGeometryToGround(pGeom1, pGeom2, pGeomBase);
			}
			else if (pGeom == pGeom1 || pGeom == pGeom2)
			{
				if (!hasRelatedConsBetweenGeometry(pGeom1, pGeomBase) && !hasRelatedConsBetweenGeometry(pGeom2, pGeomBase))
					pGeomBase->setGrounded(true);
			}
			else if (pGeom->geomType() == eSolverGeomLine)
			{
				// When drag a edge of polygon (created by mirror with symmetric on each end point), try to fix the symmetry base line
				SketchSolverLine* pLine = pGeom->solverGeom<SketchSolverLine>(eSolverGeomLine);
				if ((pLine->solverGeomStartPt() == pGeom1 || pLine->solverGeomStartPt() == pGeom2) &&
					hasSymmetryConstraintOnGeom(pLine->solverGeomEndPt()))
				{
					pGeomBase->setGrounded(true);
				}
				else if ((pLine->solverGeomEndPt() == pGeom1 || pLine->solverGeomEndPt() == pGeom2) &&
					hasSymmetryConstraintOnGeom(pLine->solverGeomStartPt()))
				{
					pGeomBase->setGrounded(true);
				}
			}
			bHandled = true;
		}
		else if ((pGeom1->geomType() == eSolverGeomCirArc && pGeom2->geomType() == eSolverGeomCirArc) ||
			(pGeom1->geomType() == eSolverGeomEllipticArc && pGeom2->geomType() == eSolverGeomEllipticArc))
		{
			if (pGeom == pGeomBase)
			{
				pGeom1->varGeomHandle()->setInvariable(true);
				pGeom2->varGeomHandle()->setInvariable(true);
				pickGeometryToGround(pGeom1, pGeom2, pGeomBase);
			}
			else if (pGeom == pGeom1 || pGeom == pGeom2)
			{
				pGeomBase->setGrounded(true);
				// For circle/ellipse, means circle center point
				if (isDragRelatedItem)
				{
					pGeom1->varGeomHandle()->setInvariable(true);
					pGeom2->varGeomHandle()->setInvariable(true);
				}
			}
			bHandled = true;
		}
	}
	BOOST_FOREACH(SketchSolverLine* pLine, linesToFixLength)
	{
		pLine->fixLength();
	}
	return bHandled;
}

void SketchVCSConstraintData::pickGeometryToGround(SketchSolverGeometry* pGeom1, 
	SketchSolverGeometry* pGeom2, 
	SketchSolverGeometry* pBaseLine)
{
	if (pGeom1->vcsBody()->isGrounded() || pGeom2->vcsBody()->isGrounded())
		return;

	if (hasRelatedConsBetweenGeometry(pGeom1, pBaseLine) 
		|| hasRelatedConsBetweenGeometry(pGeom2, pBaseLine))
		return;

	// Check whether geometry 1 is above base line
	AcGePoint3d p1 = pGeom1->position();
	AcGePoint3d basePt = pBaseLine->line().closestPointTo(p1);
	AcGeVector3d baseToP1Vec = p1 - basePt;
	if (!baseToP1Vec.isZeroLength())
	{
		baseToP1Vec.normalize();
		// If the vector is on Y axis direction, then p1 is above basePt and geometry 1 is above.
		// Ground geometry 2
		if (baseToP1Vec.dotProduct(AcGeVector3d::kYAxis) > AcGeContext::gTol.equalPoint())
		{
			pGeom2->setGrounded(true);
			return;
		}
	}
	// Ground geometry 1 if not
	pGeom1->setGrounded(true);
}

SketchVCSConstraintMapHelper::SketchVCSConstraintMapHelper(ISketchConstraint* pSketchCons, SketchVCSConstraintData* pData)
	: m_pSketchCons(pSketchCons),
	m_pData(pData)
{
	m_index = m_pData->m_solverGeomConsList.size();
}

SketchVCSConstraintMapHelper::~SketchVCSConstraintMapHelper()
{
	m_pData->addToConstraintsMap(m_pSketchCons, m_index);
}

void SketchVCSConstraintData::addToConstraintsMap(ISketchConstraint* pSketchCons, size_t consIndex)
{
	size_t curIndex = m_solverGeomConsList.size();
	if (curIndex <= consIndex)
		return;

	SolverGeomConsList list;
	for (size_t i=consIndex; i<curIndex; i++)
	{
		SketchVCSConstraintDataItem *pItem = m_solverGeomConsList[i];
		if (pSketchCons->constraintType() == pItem->consType())
			list.push_back(pItem);
	}

	m_sketchToVCSConsMap[pSketchCons] = list;
}

bool SketchVCSConstraintData::handlePolygonConsWhenDragging(SketchSolverGeometry* pPoint, SketchVCSDraggingData& dragData)
{
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (ePolygon != pItem->consType())
			continue;

		if (!pItem->containsGeom(pPoint))
			continue;

		// If dragging the polygon center
		if (pPoint == pItem->geoms().back())
		{
			SketchSolverGeometry* pFirstGeom = pItem->geoms()[0];
			if (!hasRelatedConsBetweenGeometry(pPoint, pFirstGeom))
				pFirstGeom->setGrounded(true);
		}
		else
		{
			BOOST_FOREACH(SketchSolverGeometry* pGeom, pItem->geoms())
			{
				if (pGeom->vcsBody()->isGrounded())
					return false;
			}
			BOOST_FOREACH(SketchSolverGeometry* pGeom, pItem->geoms())
			{
				dragData.addGeomToDrag(pGeom);
			}
		}
		// Set not scalable if has dimension on the line
		static bool bTest = true;
		if (bTest && pItem->patternHandle()->allowShapeChange())
		{
			size_t iLastVertex = pItem->geoms().size() - 2;
			SketchSolverGeometry* pItem1 = pItem->geoms()[iLastVertex];
			SketchSolverGeometry* pItem2 = pItem->geoms()[0];
			bool bHasConstraintWith2Points = hasRelatedConsBetweenGeometry(pItem1, pItem2, true);
			if (!bHasConstraintWith2Points)
			{
				for (size_t i=0; i<iLastVertex; i++)
				{
					SketchSolverGeometry* pItem1 = pItem->geoms()[i];
					SketchSolverGeometry* pItem2 = pItem->geoms()[i+1];
					if (hasRelatedConsBetweenGeometry(pItem1, pItem2, true))
					{
						bHasConstraintWith2Points = true;
						break;
					}
				}
			}			
			if (bHasConstraintWith2Points)
			{
				pItem->patternHandle()->setAllowShapeChange(false);
			}
		}
	}

	return true;
}

void SketchVCSConstraintData::handleConstrainedHandleWhenDragging(SketchSolverGeometry* pLine, SketchConstraintSolverContext* ctx)
{
    std::set<SketchSolverLine*> constrainedLines;
    constrainedLines.insert(pLine->solverGeom<SketchSolverLine>(SKs::Constraint::eSolverGeomLine));
    bool hasNewAdded = false;
    // search very paralleled tangents
    // even they are inter-connected via line3d
    // is it safe?
    do
    {
        hasNewAdded = false;
        std::set<SketchSolverLine*> newAdded;

        BOOST_FOREACH(SketchSolverLine* line, constrainedLines)
        {
            BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
            {
                if (!pItem->containsGeom(line))
                    continue;

                if(eParallel == pItem->consType() || ePerpendicular == pItem->consType())
                {
                    SketchSolverGeometry* pOther = line == pItem->geom1() ? pItem->geom2() : pItem->geom1();
                    SketchSolverLine* pOtherLine = pOther->solverGeom<SketchSolverLine>(SKs::Constraint::eSolverGeomLine);
                    if(std::find(constrainedLines.begin(), constrainedLines.end(), pOtherLine) != constrainedLines.end())
                    {
                        continue;
                    }

                    if(pOther->geometry()->geomType() == Geometry3D::eTangentHandle || pOther->geometry()->geomType() == Geometry3D::eLine3D)
                    {
                        newAdded.insert(pOther->solverGeom<SketchSolverLine>(SKs::Constraint::eSolverGeomLine));
                        hasNewAdded = true;
                    }
                }
            }
        }
        constrainedLines.insert(newAdded.begin(), newAdded.end());
    }
    while(hasNewAdded);

    // fix every fit point
    std::set<SketchSolverGeometry*> fitPoints;
    BOOST_FOREACH(SketchSolverLine* line, constrainedLines)
    {
        if(line->geometry()->geomType() == Geometry3D::eTangentHandle)
        {
            TangentHandle* handle = line->geometry()->query<TangentHandle>();
            fitPoints.insert(ctx->solverGeometry(handle->fitPoint()));
        }
    }

    std::set<SketchSolverGeometry*> toFix;
    BOOST_FOREACH(SketchSolverGeometry* pt, fitPoints)
    {
        bool willFix = true;
        BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
        {
            if (!pItem->containsGeom(pt))
                continue;

            if(!((pItem->consType() & eDimConstraint) || (pItem->consType() & eGeomConstraint)))
            {
                continue;
            }

            if(eLineMidPoint != pItem->consType())
            {
                willFix = false;
                break;
            }

            SketchSolverGeometry* pOther = pt == pItem->geom1() ? pItem->geom2() : pItem->geom1();
            if(pOther == 0)
            {
                continue;
            }
            if(pOther->geometry()->geomType() == Geometry3D::eTangentHandle || pOther->geometry()->geomType() == Geometry3D::eCurvatureHandle)
            {
                continue;
            }

            willFix = false;
            break;
        }
        if(willFix)
        {
            pt->setGrounded(true);
        }
    }
}

bool SketchVCSConstraintData::shouldFixLineEndPoint(SketchSolverLine* pLine, SketchSolverGeometry* pEndPt, SketchSolverGeometry* pOtherPt)
{
	// Check each constraint Item in the list
	int iConsNum = 0;
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (!pItem->containsGeom(pEndPt))
			continue;

		// No fix if has constraint between two end points
		if (pItem->isContains(pEndPt, pOtherPt))
			return false;

		iConsNum++;
		if (iConsNum > 3)
			return false;

		// Not fix if it has line mid point constraint on it (FUS-1636)
        if (eSymmetric == pItem->consType()
			|| ePolygon == pItem->consType()
            || (eLineMidPoint == pItem->consType() && pLine->geometry()->geomType() != Geometry3D::eTangentHandle))
		{
			return false;
		}
	}
	return true;
}

bool SketchVCSConstraintData::doesNeedSolveAgain(int iSolveCount)
{
	if (m_tempGroundedGeoms.empty())
		return false;
	if (iSolveCount > m_tempGroundedGeoms.size())
		return false;

	// Restore 1st group temp grounded geometries
	setTempGroundGeom(false, iSolveCount);
	
	// Apply 2nd group
	if (iSolveCount < m_tempGroundedGeoms.size())
		setTempGroundGeom(true, iSolveCount + 1);

	return true;
}

bool SketchVCSConstraintData::addTempGroundGeom(SketchSolverGeometry* pGeom, int iSolveCount)
{
	// support 3 times solving at most
	NEUTRON_ASSERT(iSolveCount <= 3);
	if (iSolveCount > 3)
		return false;

	if (m_tempGroundedGeoms.empty())
	{
		SketchSolverGeoms g1, g2, g3;
		m_tempGroundedGeoms.push_back(g1);
		m_tempGroundedGeoms.push_back(g2);
		m_tempGroundedGeoms.push_back(g3);
	}

	m_tempGroundedGeoms[iSolveCount-1].push_back(pGeom);
	return true;
}

bool SketchVCSConstraintData::setTempGroundGeom(bool isSetGround, int iSolveCount)
{
	NEUTRON_ASSERT(iSolveCount <= m_tempGroundedGeoms.size());
	if (iSolveCount > m_tempGroundedGeoms.size())
		return false;
	SketchSolverGeoms& geoms = m_tempGroundedGeoms[iSolveCount-1];
	BOOST_FOREACH(SketchSolverGeometry* pGeom, geoms)
	{
		pGeom->setGrounded(isSetGround);
	}
	return true;
}

bool SketchVCSConstraintData::tempGroundGeomesPreSolve(ISketchConstraint* pSketchCons, SketchConstraintSolverContext& ctx)
{
	// For symmetry constraint
	if (pSketchCons->constraintType() == SKs::eSymmetric)
	{
		if (pSketchCons->geometries().size() != 3)
		{
			NEUTRON_ASSERT(false);
			return false;
		}
		SketchSolverGeometry* pGeom1 = ctx.solverGeometry(pSketchCons->geometries()[0]);
		SketchSolverGeometry* pGeom2 = ctx.solverGeometry(pSketchCons->geometries()[1]);
		SketchSolverGeometry* pGeomBase = ctx.solverGeometry(pSketchCons->geometries()[2]);
		
		// Skip if one is already fixed
		if (!pGeom1->isGrounded() && !pGeom2->isGrounded())
		{
			addTempGroundGeom(pGeom1);
			addTempGroundGeom(pGeomBase);

			addTempGroundGeom(pGeom2, 2);
			addTempGroundGeom(pGeomBase, 2);

			addTempGroundGeom(pGeomBase, 3);
		}
	}

	if (!m_tempGroundedGeoms.empty())
		return setTempGroundGeom(true, 1);

	return false;
}

ISketchConstraint* SketchVCSConstraintData::newAddConstraint()
{
	return m_pNewAddConstraint;
}

void SketchVCSConstraintData::newAddConstraint(ISketchConstraint* pNewAddCons)
{
	m_pNewAddConstraint = pNewAddCons;
}

bool SketchVCSConstraintData::preferMovingLaterCreatedGeom()
{
	if (NULL == geom1() || NULL == geom2())
		return false;

	PassiveRef<Geometry3D> rGeom1 = geom1()->geometry();
	PassiveRef<Geometry3D> rGeom2 = geom2()->geometry();
	if (!rGeom1 || !rGeom2)
		return false;

	// Swap them to make later created geometry in 2nd, so solver will move it by default
	if (rGeom1->laterCreatedThan(rGeom2))
	{
		SketchSolverGeometry* pGeomTemp = geom1();
		geom1(geom2());
		geom2(pGeomTemp);
	}

	return true;
}

bool SketchVCSConstraintData::hasSymmetryConstraintOnGeom(SketchSolverGeometry* pGeom)
{
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (eSymmetric != pItem->consType())
			continue;

		if (pItem->containsGeom(pGeom))
			return true;
	}
	return false;
}

void SketchVCSConstraintData::addCoincidentPointsToDrag(SketchSolverGeometry* pGeom, SketchVCSDraggingData& dragData)
{
	BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, m_solverGeomConsList)
	{
		if (eCoincident != pItem->consType())
			continue;
		if (pItem->containsGeom(pGeom))
		{
			SketchSolverGeometry* pPt = pItem->geom1();
			if (pItem->geom1() == pGeom)
				pPt = pItem->geom2();
			if (pPt == NULL)
				continue;
			if (pPt->geomType() != eSolverGeomPoint)
				continue;

			dragData.addGeomToDrag(pPt);
		}
	}
}