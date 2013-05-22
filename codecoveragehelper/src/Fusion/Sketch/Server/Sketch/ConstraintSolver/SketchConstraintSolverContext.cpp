#include "Pch/SketchPch.h"
#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchSolverGeometry.h"
#include "SketchSolverPoint.h"
#include "SketchSolverLine.h"
#include "SketchSolverCircArc.h"
#include "SketchSolverEllipticArc.h"
#include "SketchSolverSpline.h"
#include "SketchVCSStatus.h"
#include "SketchVCSExt.h"
#include "../Utils/SketchLogMgr.h"

#include "../Objects/ISketchConstraint.h"
#include "../Objects/SketchGeomConstraint.h"

#include <Server/Base/Assert/Assert.h>
#include <Server/Base/Assert/BugAlert.h>
#include <VCSAPI.h>

// Geometries
#include <Server/Geometry/Geometry3D/Geometry3D.h>
#include <Server/Geometry/Geometry3D/Line3D.h>
#include <Server/Geometry/Geometry3D/CircArc3D.h>
#include <Server/Geometry/Geometry3D/EllipticArc3D.h>
#include <Server/Geometry/Geometry3D/Point3D.h>
#include <Server/Geometry/Geometry3D/Spline3D.h>
#include <Server/Geometry/Geometry3D/Point3D.h>
#include <Server/Geometry/Geometry3D/TangentHandle.h>
#include <Server/Geometry/Geometry3D/CurvatureHandle.h>

using namespace Ns;
using namespace SKs;
using namespace SKs::Constraint;
using namespace SKs::Geometry;
using namespace Ns::Geometry;

SketchConstraintSolverContext::SketchConstraintSolverContext()
	: m_rSketch(), 
	m_geomToSolverGeomMap(),
	m_pVCSInterface(NULL),
	m_updatedPoints(),
	m_updatedCurves(),
	m_consData(),
	m_dragData(),
	m_newAddedCons(),
	m_bIsDragging(false),
	m_pStatus(new SketchVCSStatus())
{
	// Use 1.0e-6, same as ASM used in sketch profile building
	m_tol.setEqualPoint(1.0e-6);
	m_tol.setEqualVector(1.0e-6);
}

SketchConstraintSolverContext::~SketchConstraintSolverContext()
{
	cleanup();

	delete m_pStatus;
	m_pStatus = NULL;
}

void SketchConstraintSolverContext::cleanup()
{
	m_rSketch = NULL;

	// Delete solver geometries
	GeomToSolverGeomMap::iterator iter = m_geomToSolverGeomMap.begin();
	for (; iter != m_geomToSolverGeomMap.end(); iter++)
	{
		SketchSolverGeometry* pGeom = iter->second;
		delete pGeom;
		pGeom = NULL;
	}
	m_geomToSolverGeomMap.clear();

	if (NULL != m_pVCSInterface)
	{
		delete m_pVCSInterface;
		m_pVCSInterface = NULL;
	}

	m_updatedPoints.clear();
	m_updatedCurves.clear();
	m_consData.cleanup();
	m_dragData.cleanup();
	m_newAddedCons.clear();
	m_pStatus->cleanup();	
}

SketchVCSConstraintData& SketchConstraintSolverContext::consData()
{
	return m_consData;
}

SketchVCSDraggingData& SketchConstraintSolverContext::dragData()
{
	return m_dragData;
}

bool SketchConstraintSolverContext::needVCSSolve(PassiveRef<MSketch> rSketch, ISketchConstraint* pCons)
{
	if(pCons && (pCons->constraintType() == SKs::eTangent || pCons->constraintType() == SKs::eSmooth))
	{
		const std::vector<PassiveRef<Geometry3D> >& geoms = pCons->geometries();
		if(geoms.size() == 2)
		{
            // both spline, no need vcs
            // if both has tangents on same point, still need vcs
            if((geoms[0]->geomType() == Geometry3D::eSpline3D && !geoms[0]->isFixed())
                && (geoms[1]->geomType() == Geometry3D::eSpline3D && !geoms[1]->isFixed()))
			{
                CurveContinuityConstraint* ccc = dynamic_cast<CurveContinuityConstraint*>(pCons);
                Spline3D* sp1 = geoms[0]->query<Spline3D>();
                Spline3D* sp2 = geoms[1]->query<Spline3D>();

                bool need1 = ccc->connectedAtStart1() ? sp1->hasTangent(0) : sp1->hasTangent(sp1->fitPoints() - 1);
                bool need2 = ccc->connectedAtStart2() ? sp2->hasTangent(0) : sp2->hasTangent(sp2->fitPoints() - 1);
				return need1 && need2;
			}

            // smooth - g2 continuity, no need vcs
            if(((geoms[0]->geomType() == Geometry3D::eSpline3D && !geoms[0]->isFixed())
                || (geoms[1]->geomType() == Geometry3D::eSpline3D && !geoms[1]->isFixed())) && pCons->constraintType() == SKs::eSmooth)
            {
                return false;
            }

            if((geoms[0]->geomType() == Geometry3D::eSpline3D && !geoms[0]->isFixed())
                || (geoms[1]->geomType() == Geometry3D::eSpline3D && !geoms[1]->isFixed()))
            {
                Spline3D* sp = geoms[0]->geomType() == Geometry3D::eSpline3D ? geoms[0]->query<Spline3D>() : geoms[1]->query<Spline3D>();
                Curve3D* c = geoms[0]->geomType() == Geometry3D::eSpline3D ? geoms[1]->query<Curve3D>() : geoms[0]->query<Curve3D>();
                bool atStart = false;
                bool outward = false;
                Spline3D::EContinuityFlag startFlag = Spline3D::eNone;
                Spline3D::EContinuityFlag endFlag = Spline3D::eNone;
                PassiveRef<Curve3D> startCurve = sp->startContinuityCurve(atStart, outward, startFlag);
                PassiveRef<Curve3D> endCurve = sp->endContinuityCurve(atStart, outward, endFlag);

                if(startFlag == Spline3D::eTangentContinuity && startCurve == c && sp->hasTangent(0))
                {
                    return true;
                }

                if(endFlag == Spline3D::eTangentContinuity && endCurve == c && sp->hasTangent(sp->fitPoints() - 1))
                {
                    return true;
                }

                return false;
            }
		}
	}
    return true;
}

bool SketchConstraintSolverContext::solve(PassiveRef<MSketch> rSketch, ISketchConstraint* pCons)
{
	SKETCHLOG_TIME("SketchConstraintSolverContext::solve");
	SKETCHLOG_MEMORY("SketchConstraintSolverContext::solve_1");

    if(!needVCSSolve(rSketch, pCons))
    {
        return true;
    }

	std::vector< PassiveRef<Geometry3D> > geomtriesToDrag;
	if (!initialize(rSketch, geomtriesToDrag, pCons))
		return false;

	if (NULL == pCons)
	{
#ifdef _DEBUG
		static bool bUseOldSolveMethod = false;
		if (bUseOldSolveMethod)
		{
			bool res = m_pVCSInterface->solve(0, false);
			NEUTRON_ASSERT(res);
			if (!res)
				return false;
			return updateSolverGeometries(true);
		}
#endif
		return solveByCheckingUnmeetConstraints(NULL);
	}

	bool res1 = m_consData.tempGroundGeomesPreSolve(pCons, *this);

	bool res = m_pVCSInterface->minMovementSolve(m_newAddedCons, pCons->constraintType());

	if (m_pStatus->status() == kInconsistent)
	{
		// check whether has multi un-meet constraints
		if (solveByCheckingUnmeetConstraints(pCons))
			return true;

		int iSolveCount = 1;
		while (!res && m_consData.doesNeedSolveAgain(iSolveCount))
		{
			iSolveCount++;
			res = m_pVCSInterface->minMovementSolve(m_newAddedCons, pCons->constraintType());
		}
	}

#ifdef _DEBUG
	if (!res)
		dumpSolverData();
#endif	
	if (!res)
		return false;

	return updateSolverGeometries(true);
}

void SketchConstraintSolverContext::addGeomToList(const PassiveRef<Geometry3D>& rGeom,
	std::vector< PassiveRef<Geometry3D> >& listToCheckAdd,
	std::vector< PassiveRef<Geometry3D> >& listToAdd,
	std::vector< PassiveRef<Geometry3D> >& listToAdd2)
{
	if (listToCheckAdd.end() != std::find(listToCheckAdd.begin(), listToCheckAdd.end(), rGeom))
		return;
	listToCheckAdd.push_back(rGeom);
	listToAdd.push_back(rGeom);
	listToAdd2.push_back(rGeom);
}

void SketchConstraintSolverContext::getConnectedGeometries(const PassiveRef<Geometry3D>& targetGeom, 
	std::vector< PassiveRef<Geometry3D> >& alreadyAddedGeoms,
	std::vector< PassiveRef<Geometry3D> >& outputGeoms)
{
	std::vector< PassiveRef<Geometry3D> > geomsToAdd;
	if (PassiveRef<Point3D> rPoint = targetGeom->query<Point3D>())
	{
		BOOST_FOREACH(PassiveRef<Curve3D>& curve, rPoint->curveUses()->curves())
		{
			addGeomToList(curve, alreadyAddedGeoms, outputGeoms, geomsToAdd);
		}
	}
	else if (PassiveRef<Curve3D> rCurve = targetGeom->query<Curve3D>())
	{
		std::vector< PassiveRef<Point3D> > items = rCurve->deleteNominees();
		BOOST_FOREACH(PassiveRef<Point3D>& item, items)
		{
			PassiveRef<Geometry3D> geom = item;
			addGeomToList(geom, alreadyAddedGeoms, outputGeoms, geomsToAdd);
		}
	}

	BOOST_FOREACH(PassiveRef<Geometry3D>& rGeom, geomsToAdd)
	{
		getConnectedGeometries(rGeom, alreadyAddedGeoms, outputGeoms);
	}
}

bool SketchConstraintSolverContext::getTargetGeomsForSolving(const std::vector< PassiveRef<Geometry3D> >& inputGeometries, 
	std::vector<ISketchConstraint*>& sketchAllCons,
	std::vector< PassiveRef<Geometry3D> >& targetGeoms)
{
	// If too many geometries (such as dragging lots of geometries), use old logic (populating all constraints from sketch),
	// this is to avoid performance issue, current use temp number 8, to be verified (zhanglo)
	if (static_cast<int>(inputGeometries.size()) >= 8)
		return false;

	targetGeoms.assign(inputGeometries.begin(), inputGeometries.end());
	BOOST_FOREACH(const PassiveRef<Geometry3D>& rInputGeom, inputGeometries)
	{
		BOOST_FOREACH(ISketchConstraint* pItem, sketchAllCons)
		{
			const std::vector< PassiveRef<Geometry3D> >& consGeoms = pItem->geometries();
			if (consGeoms.end() == std::find(consGeoms.begin(), consGeoms.end(), rInputGeom))
				continue;

			// Add geom (consGeoms should have at most 3 geometries)
			BOOST_FOREACH(const PassiveRef<Geometry3D>& rRelatedGeom, consGeoms)
			{
				if (targetGeoms.end() != std::find(targetGeoms.begin(), targetGeoms.end(), rRelatedGeom))
					continue;
				targetGeoms.push_back(rRelatedGeom);
			}
		}
	}

	return true;
}

PassiveRef<MSketch> SketchConstraintSolverContext::sketch() const
{
    return m_rSketch;
}

void SketchConstraintSolverContext::getAssociatedConstraints(PassiveRef<MSketch> rSketch,
	std::vector<ISketchConstraint*> &associatedCons, 
	ISketchConstraint* pNewCons,
	const std::vector< PassiveRef<Geometry3D> >& geomtriesToDrag)
{
	// Get all kinds of constraints from sketch
	// Use old way by populating all constraints if current has small amount of constraints
	// Use temporary number "10" which is to be verified (zhanglo)
	rSketch->getAllSketchConstraints(associatedCons);
	int iConsNum = static_cast<int>(associatedCons.size());
	static int iMaxNum = 10;
	if (iConsNum <= iMaxNum)
		return;

	// Use old way if not adding new constraint or dragging
	if (NULL == pNewCons && geomtriesToDrag.empty())
		return;

	// Get target geometries from the new constraint (that need to do solving) or dragging geometries
	std::vector< PassiveRef<Geometry3D> > targetGeoms;
	if (NULL != pNewCons)
	{
		if (!getTargetGeomsForSolving(pNewCons->geometries(), associatedCons, targetGeoms))
			return;
	}
	else
	{
		if (!getTargetGeomsForSolving(geomtriesToDrag, associatedCons, targetGeoms))
			return;
	}

	// Prepare constraints
	std::vector<ISketchConstraint*> constraintList;
	constraintList.assign(associatedCons.begin(), associatedCons.end());
	associatedCons.clear();	

	// Call recur method to get associated constraints
	std::vector< PassiveRef<Geometry3D> > associatedGeoms;
	BOOST_FOREACH(PassiveRef<Geometry3D>& geom, targetGeoms)
	{
		if (constraintList.empty())
			break;
		NEUTRON_ASSERT(geom.isNotNull());
		if (geom.isNull())
			continue;
		getAssociatedConstraintsItem(associatedCons, associatedGeoms, constraintList, geom);
	}
}

void SketchConstraintSolverContext::getAssociatedConstraintsItem(std::vector<ISketchConstraint*> &associatedCons, 
	std::vector< PassiveRef<Geometry3D> >& associatedGeoms,
	std::vector<ISketchConstraint*>& constraintList,
	PassiveRef<Geometry3D>& targetGeom)
{
	if (constraintList.empty())
		return;

	NEUTRON_ASSERT(targetGeom.isNotNull());
	if (targetGeom.isNull())
		return;

	// #. Get constraints that associated with the target geometry
	std::vector<ISketchConstraint*> consToAdd;
	BOOST_FOREACH(ISketchConstraint* pItem, constraintList)
	{
		const std::vector< PassiveRef<Geometry3D> >& consGeoms = pItem->geometries();
		if (consGeoms.end() == std::find(consGeoms.begin(), consGeoms.end(), targetGeom))
			continue;

		consToAdd.push_back(pItem);

		// Add to associated cons list
		if (associatedCons.end() == std::find(associatedCons.begin(), associatedCons.end(), pItem))
			associatedCons.push_back(pItem);
	}

	// #. Get all connected geometries with the target geometry
	associatedGeoms.push_back(targetGeom);
	std::vector< PassiveRef<Geometry3D> > directAssociatedGeoms;
	getConnectedGeometries(targetGeom, associatedGeoms, directAssociatedGeoms);

	// #. Handle the found associated constraint (cache them, get geometries from them, etc)
	BOOST_FOREACH(ISketchConstraint* pItem, consToAdd)
	{
		std::vector<ISketchConstraint*>::iterator resultIt = std::find(constraintList.begin(), constraintList.end(), pItem);
		NEUTRON_ASSERT(constraintList.end() != resultIt);
		if (constraintList.end() != resultIt)
			constraintList.erase(resultIt);

		// Add to associated geom list
		BOOST_FOREACH(const PassiveRef<Geometry3D>& geom, pItem->geometries())
		{
			if (geom == targetGeom)
				continue;
			if (associatedGeoms.end() != std::find(associatedGeoms.begin(), associatedGeoms.end(), geom))
				continue;
			associatedGeoms.push_back(geom);
			directAssociatedGeoms.push_back(geom);
		}
	}

	// #. Recur to call others
	BOOST_FOREACH(PassiveRef<Geometry3D>& geom, directAssociatedGeoms)
	{
		getAssociatedConstraintsItem(associatedCons, associatedGeoms, constraintList, geom);
	}
}

bool SketchConstraintSolverContext::initialize(PassiveRef<MSketch>& rSketch, 
	const std::vector< PassiveRef<Geometry3D> >& geomtriesToDrag,
	ISketchConstraint* pNewAddCons)
{
	cleanup();

	m_rSketch = rSketch;
	m_pVCSInterface = new SketchVCSInterface(m_pStatus);
	m_bIsDragging = !geomtriesToDrag.empty();

	bool bCheckInvalidConstraint = true;
	if (NULL == pNewAddCons)
		bCheckInvalidConstraint = false;

	// Get related constraints
	std::vector<ISketchConstraint*> constraintList;
	getAssociatedConstraints(rSketch, constraintList, pNewAddCons, geomtriesToDrag);	

	// Use combined body (current disable it, double check later)
	static bool enableCombineBody = false;
	if (m_bIsDragging && enableCombineBody)
	{
		BOOST_FOREACH(ISketchConstraint* pCons, constraintList)
		{
			if (pCons->constraintType() != SKs::eLinearDim)
				continue;
			if (pCons->geometries().size() != 1)
				continue;

			PassiveRef<Geometry3D> rGeom = pCons->geometries()[0];
			SketchSolverGeometry* pGeom = solverGeometry(rGeom);
			if (!pGeom)
				return false;
			if (pGeom->geomType() != eSolverGeomLine)
				continue;

			pGeom->useCombinedBody(true);
		}
	}

	// Add new constraint
	if (NULL != pNewAddCons)
	{
		m_consData.newAddConstraint(pNewAddCons);
		m_consData.isAddingNewConstraint(true);
		bool res = createConstraint(pNewAddCons);
		m_consData.isAddingNewConstraint(false);

		m_consData.newAddedConsHandle(m_newAddedCons);
		NEUTRON_ASSERT(res && !m_newAddedCons.empty());
		if (!res || m_newAddedCons.empty())
			return false;
	}

	// Add other constraints
	BOOST_FOREACH(ISketchConstraint* pCons, constraintList)
	{
		// Skip the constraint that is already created (such as editing a constraint)
		if (pNewAddCons == pCons)
			continue;

		bool res = createConstraint(pCons, bCheckInvalidConstraint);
		// Skip the sick constraint in the solving, and put an alert here
		NEUTRON_BUG_ALERT_BOOL(res);
		if (!res)
			continue;
	}

	// Create solver geometry for geometries to drag
	BOOST_FOREACH(PassiveRef<Geometry3D> rGeom, geomtriesToDrag)
	{
		SketchSolverGeometry* pGeom = solverGeometry(rGeom);
		NEUTRON_ASSERT(NULL != pGeom);
	}

	addAdditionalConstraints();

	// Add verification constraints if current is dragging
	// For normal solving, use a different way by adding VCS_GE constraint to avoid degenerate
	if (m_bIsDragging)
		m_pVCSInterface->vcsSystem()->addExtVerificationCon(new SketchVCSExtVerificationCon(this));

	SKETCHLOG_SOLVER_INFO(this);

	return true;
}

void SketchConstraintSolverContext::addAdditionalConstraints()
{
	// Add additional associated constraints (may need populate all geometries from sketch: TBD)
	//
	// Get all geoms from map
	std::vector<SketchSolverGeometry*> tempSolverGeomList;
	GeomToSolverGeomMap::iterator iter = m_geomToSolverGeomMap.begin();	
	for (; iter != m_geomToSolverGeomMap.end(); iter++)
	{
		SketchSolverGeometry* pGeom = iter->second;
		NEUTRON_ASSERT(NULL != pGeom);
		if (NULL == pGeom)
			continue;
		tempSolverGeomList.push_back(pGeom);
	}
	// Add additional constraints
	BOOST_FOREACH(SketchSolverGeometry* pSolverGeom, tempSolverGeomList)
	{
		bool res = pSolverGeom->addAdditionalConstraints(m_consData);
		NEUTRON_ASSERT(res);
	}
	// Add on plane constraints
	iter = m_geomToSolverGeomMap.begin();	
	for (; iter != m_geomToSolverGeomMap.end(); iter++)
	{
		SketchSolverGeometry* pGeom = iter->second;
		if (eSolverGeomPoint == pGeom->geomType())
		{
			pGeom->addOnPlaneConstraint(m_consData);
			continue;
		}
		m_consData.resetData(pGeom);
		bool res = m_pVCSInterface->onPlaneConstraint(m_consData);
		NEUTRON_ASSERT(res);
	}
}

bool SketchConstraintSolverContext::splinesCreated(const std::vector<PassiveRef<Geometry3D> >& geoms, std::vector<bool>& created)
{
    if(geoms.size() != 2)
    {
        return false;
    }
    created.clear();
	BOOST_FOREACH(const PassiveRef<Geometry3D>& rGeom, geoms)
    {
        if(rGeom->geomType() != Geometry3D::eSpline3D)
        {
            return false;
        }
        GeomToSolverGeomMap::iterator iter = m_geomToSolverGeomMap.find(rGeom);
        bool alreadyHas = iter != m_geomToSolverGeomMap.end();
        created.push_back(alreadyHas);
    }
    return true;
}

bool SketchConstraintSolverContext::setVCSConstraintData(ISketchConstraint* pCons)
{
	NEUTRON_ASSERT(NULL != pCons);
	if (NULL == pCons)
		return false;
	NEUTRON_ASSERT(pCons->geometries().size() > 0);
	if (pCons->geometries().size() == 0)
		return false;
    
	// Get geometries from constraint
	std::vector<SketchSolverGeometry*> geoms;
	BOOST_FOREACH(const PassiveRef<Geometry3D>& rGeom, pCons->geometries())
	{
		// Skip sick constraint/dimension
		NEUTRON_ASSERT(rGeom.isNotNull());
		if (rGeom.isNull())
			return false;
		SketchSolverGeometry* pGeom = solverGeometry(rGeom);
		geoms.push_back(pGeom);
	}
    // VCS does not support oppsite direction tangent constraint
    // so in this case we will have to reverse the ext curve
	std::vector<bool> created;
	bool needCheckReverse = splinesCreated(pCons->geometries(), created);
    if(needCheckReverse)
    {
        // if both curves are created
        // we will reverse second curve
        if(created[0] && created[1])
        {        
            CurveContinuityConstraint* cons = dynamic_cast<CurveContinuityConstraint*>(pCons);
            if(cons)
            {
                if(cons->connectedAtStart1() == cons->connectedAtStart2())
                {
                    std::vector< ISketchConstraint* > allConsts;
                    m_rSketch->getAllSketchConstraints(allConsts);

                    std::set<SketchSolverGeometry*> visited;
                    std::vector<SketchSolverGeometry*> remains;
                    remains.push_back(geoms[1]);
                    visited.insert(geoms[0]);
                    while(remains.size() > 0)
                    {
                        SketchSolverGeometry* pos = remains[0];
                        SketchSolverSpline* sp = pos->solverGeom<SketchSolverSpline>(eSolverGeomSpline);
                        if(visited.find(pos) != visited.end())
                        {
                            remains.erase(remains.begin());
                            continue;
                        }
                        visited.insert(pos);
                        remains.erase(remains.begin());
                        BOOST_FOREACH(ISketchConstraint* pConst, allConsts)
                        {
                            if(pConst->constraintType() == SKs::eTangent || pConst->constraintType() == SKs::eSmooth)
                            {
                                SketchSolverGeometry* connected = 0;
                                const std::vector<PassiveRef<Geometry3D> > children = pConst->geometries();
                                bool found = false;
                                BOOST_FOREACH(PassiveRef<Geometry3D> g, children)
                                {
                                    if(g == sp->geometry())
                                    {
                                        found = true;
                                        continue;
                                    }
                                    connected = solverGeometry(g);
                                }
                                if(found)
                                {
                                    SketchSolverSpline* connectedSp = connected->solverGeom<SketchSolverSpline>(eSolverGeomSpline);
                                    connectedSp->needReverse(!connectedSp->needReverse());
                                    remains.push_back(connectedSp);
                                }
                            }
                        }
                    }
                }
            }
        }
        else 
        {
            int index = -1;
            if(created[0] && !created[1])
            {
                index = 1;
            }
            else if(!created[0] && created[1])
            {
                index = 0;
            }
            else if(!created[0] && !created[1])
            {
                index = 1;
            }
            CurveContinuityConstraint* cons = dynamic_cast<CurveContinuityConstraint*>(pCons);
            if(cons)
            {
                if(cons->connectedAtStart1() == cons->connectedAtStart2())
                {
                    SketchSolverSpline* sp = dynamic_cast<SketchSolverSpline*>(geoms[index]);
                    sp->needReverse(true);
                }
            }
        }
    }

    // Set value
	m_consData.resetData(geoms, pCons->constraintValue(), pCons->constraintType());

	return true;
}

bool SketchConstraintSolverContext::createConstraint(ISketchConstraint* pCons, bool bCheckInValidCons)
{
	if (!setVCSConstraintData(pCons))
		return false;

	// Check whether the constraint is invalid (need to be solved, such as mid-point constraint)
	if (bCheckInValidCons)
	{
		bool bRes = m_pVCSInterface->checkInValidConstraint(m_consData);
		NEUTRON_ASSERT(bRes);
		// If has invalid constraint, skip it for temporary to avoid bad performance
		// We need to make sure no invalid constraint when do solving (zhanglo)
		if (m_consData.hasInvalidConstraint())
			return true;
	}

	// Create related constraints
	//
	SketchVCSConstraintMapHelper consMap(pCons, &m_consData);

	switch (pCons->constraintType())
	{
	case eFixed:
		return m_pVCSInterface->fixConstraint(m_consData);
		break;
	case eCoincident:
		return m_pVCSInterface->coincidentConstraint(m_consData);
		break;
	case eCollinear:
		return m_pVCSInterface->collinearConstraint(m_consData);
		break;
	case eConcentric:
		return m_pVCSInterface->concentricConstraint(m_consData);
		break;
	case eParallel:
		return m_pVCSInterface->parallelConstraint(m_consData);
		break;
	case ePerpendicular:
		return m_pVCSInterface->perpendicularConstraint(m_consData);
		break;
	case eVertical:
		return m_pVCSInterface->verticalConstraint(m_consData);
		break;
	case eHorizontal:
		return m_pVCSInterface->horizontalConstraint(m_consData);
		break;
	case eLineMidPoint:
		return m_pVCSInterface->midPointConstraint(m_consData);
		break;
	case eEqual:
		return m_pVCSInterface->equalConstraint(m_consData);
		break;
	case eTangent:
    case eSmooth:
        {
		    if(!m_pVCSInterface->tangentConstraint(m_consData))
            {
                return false;
            }
            SketchSolverGeometry* c = m_consData.geom1()->geomType() == SKs::Constraint::eSolverGeomSpline ? m_consData.geom2() : m_consData.geom1();
            if(c)
            {
                c->addExtraHandlesForSolving(m_consData, dynamic_cast<CurveContinuityConstraint*>(pCons));
            }
            return true;
        }
		break;
	case eLinearDim:
		return m_pVCSInterface->linearDimensionConstraint(m_consData);
		break;
	case eHorizontalDim:
		return m_pVCSInterface->alignAxisDimensionConstraint(m_consData, AcGeVector3d::kYAxis);
		break;
	case eVerticalDim:
		return m_pVCSInterface->alignAxisDimensionConstraint(m_consData, AcGeVector3d::kXAxis);
		break;
	case eRadialDim:
		return m_pVCSInterface->circleRadiusConstraint(m_consData);
		break;
	case eDiameterDim:
		return m_pVCSInterface->circleDiameterConstraint(m_consData);
		break;
	case eAngularDim:
		m_consData.preferMovingLaterCreatedGeom();
		return m_pVCSInterface->angleConstraint(m_consData);
		break;
	case eEllipseMajorRadiusDim:
		return m_pVCSInterface->ellipseRadiusConstraint(m_consData);
		break;
	case eEllipseMinorRadiusDim:
		return m_pVCSInterface->ellipseRadiusConstraint(m_consData, NULL, false);
		break;
	case ePolygon:
		return m_pVCSInterface->polygonConstraint(m_consData);
		break;
	case eSymmetric:
		return m_pVCSInterface->symmetryConstraint(m_consData);
		break;
	default:
		break;
	}

	NEUTRON_ASSERT(false && _DNGH("Not supported yet!"));
	return false;
}

SketchSolverGeometry* SketchConstraintSolverContext::solverGeometry(const PassiveRef<Geometry3D>& rGeom)
{
	if (rGeom.isNull())
		return NULL;

	GeomToSolverGeomMap::iterator iter = m_geomToSolverGeomMap.find(rGeom);
	if (iter != m_geomToSolverGeomMap.end())
	{
		return iter->second;
	}

	// Create one if current hasn't
	//
	SketchSolverGeometry* pSolverGeom = createSolverGeometry(rGeom);
	NEUTRON_ASSERT(NULL != pSolverGeom);
	if (NULL == pSolverGeom)
		return NULL;

	// Add to map
	m_geomToSolverGeomMap.insert(GeomToSolverGeomMap::value_type(rGeom, pSolverGeom));

	return pSolverGeom;
}

SketchSolverGeometry* SketchConstraintSolverContext::createSolverGeometry(PassiveRef<Geometry3D> rGeom)
{
	PassiveRef<Point3D> point = dynamic_cast_PassiveRef<Point3D>(rGeom);
	if (point.isNotNull())
	{
		return new SketchSolverPoint(point, this);
	}
	PassiveRef<Line3D> line = dynamic_cast_PassiveRef<Line3D>(rGeom);
	if (line.isNotNull())
	{
		return new SketchSolverLine(line, this);
	}
	PassiveRef<CircArc3D> circle = dynamic_cast_PassiveRef<CircArc3D>(rGeom);
	if (circle.isNotNull())
	{
		return new SketchSolverCircArc(circle, this);
	}
	PassiveRef<EllipticArc3D> ellipse = dynamic_cast_PassiveRef<EllipticArc3D>(rGeom);
	if (ellipse.isNotNull())
	{
		return new SketchSolverEllipticArc(ellipse, this);
	}
	PassiveRef<Spline3D> spline = dynamic_cast_PassiveRef<Spline3D>(rGeom);
	if (spline.isNotNull())
	{
		return new SketchSolverSpline(spline, this);
	}

	return NULL;
}

/// Get VCSInterface
SketchVCSInterface* SketchConstraintSolverContext::vcsInterface()
{
	return m_pVCSInterface;
}

void SketchConstraintSolverContext::checkSolverResults()
{
	static bool isgo = false;
	if (!isgo)
		return;

	// Check tangent
	// get line1, line2, circle1
	AcGeLine3d line1, line2;
	AcGeCircArc3d circle1;
	bool bSetline1 = false;
	bool bSetline2 = false;
	bool bSetCircle1 = false;

	GeomToSolverGeomMap::iterator iter = m_geomToSolverGeomMap.begin();
	for (; iter != m_geomToSolverGeomMap.end(); iter++)
	{
		SketchSolverGeometry* pGeom = iter->second;
		if (pGeom->geomType() == eSolverGeomLine)
		{
			if (!bSetline1)
			{
				line1 = pGeom->line();
				bSetline1 = true;
			}
			else if (!bSetline2)
			{
				line2 = pGeom->line();
				bSetline2 = true;
			}
		}
		else if (!bSetCircle1 && pGeom->geomType() == eSolverGeomCirArc)
		{
			circle1 = pGeom->circArc();
			bSetCircle1 = true;
		}
	}
	if (!bSetline1 || !bSetline2 || !bSetCircle1)
		return;

	int intn = 0;
	AcGePoint3d p1, p2;
	if (circle1.intersectWith(line1, intn, p1, p2))
	{
		NEUTRON_ASSERT(intn == 1);
	}
	if (circle1.intersectWith(line2, intn, p1, p2))
	{
		NEUTRON_ASSERT(intn == 1);
	}
}

bool SketchConstraintSolverContext::updateSolverGeometries(bool bSolveSuccess, bool bSkipUpdatedCurveInSolve)
{
#ifdef _DEBUG
	checkSolverResults();
#endif

	m_updatedPoints.clear();
	m_updatedCurves.clear();

	// Below codes could be removed when vcs fix the issue
	// Make sure not return kSolved if verification fails: drag mid point of line which is already degenerated
	if (!isRelationMet())
		bSolveSuccess = false;

	// Update geometry value
	GeomToSolverGeomMap::iterator iter = m_geomToSolverGeomMap.begin();
	for (; iter != m_geomToSolverGeomMap.end(); iter++)
	{
		SketchSolverGeometry* pGeom = iter->second;
		bool res = pGeom->updateValueBySolvedBody(!bSolveSuccess);
		NEUTRON_ASSERT(res);
	}

	// Get curves that need to update
	BOOST_FOREACH(PassiveRef<Point3D> pt, m_updatedPoints)
	{
		StrongRef<CurveUses> curveUses = pt->curveUses();
		std::vector< PassiveRef<Curve3D> >& curveList = curveUses->curves();
		BOOST_FOREACH(PassiveRef<Curve3D> curve, curveList)
		{
			if (std::find(m_updatedCurves.begin(), m_updatedCurves.end(), curve) != m_updatedCurves.end())
				continue;
			m_updatedCurves.push_back(curve);
            TangentHandle* tangent = curve->query<TangentHandle>();
            if(tangent)
            {
                StrongRef<CurveUses> splines = tangent->curveUses();
                BOOST_FOREACH(PassiveRef<Curve3D> sp, splines->curves())
                {
                    m_updatedCurves.push_back(sp);
                }
            }
            CurvatureHandle* curvature = curve->query<CurvatureHandle>();
            if(curvature)
            {
                StrongRef<CurveUses> splines = curvature->curveUses();
                BOOST_FOREACH(PassiveRef<Curve3D> sp, splines->curves())
                {
                    m_updatedCurves.push_back(sp);
                }
            }
	
		}
	}

	std::vector<ISketchConstraint*> associatedCons;
	m_rSketch->getAllSketchConstraints(associatedCons);

    std::vector<PassiveRef<Curve3D> > toUpdate = m_updatedCurves;
    std::vector<PassiveRef<Curve3D> > newAdded;

    std::vector<CurveContinuityConstraint*> cccs;
    // find all curve continuity constraint
    BOOST_FOREACH(ISketchConstraint* constraint, associatedCons)
    {
        CurveContinuityConstraint* ccc = dynamic_cast<CurveContinuityConstraint*>(constraint);
        if(ccc)
        {
            cccs.push_back(ccc);
        }
    }

    do
    {
        newAdded.clear();
        BOOST_FOREACH(CurveContinuityConstraint* ccc, cccs)
        {
            BOOST_FOREACH(PassiveRef<Curve3D> curve, toUpdate)
            {
                if(ccc->containsGeometry(curve))
                {
                    const std::vector< PassiveRef<Geometry3D> >& geoms = ccc->geometries();
                    BOOST_FOREACH(PassiveRef<Geometry3D> g, geoms)
                    {
                        PassiveRef<Curve3D> c = g->query<Curve3D>();
                        if(c.isNull() || c->geomType() != Geometry3D::eSpline3D)
                        {
                            continue;
                        }
                        if(c.target() == curve.target())
                        {
                            continue;
                        }
                        if(std::find(toUpdate.begin(), toUpdate.end(), c) != toUpdate.end())
                        {
                            continue;
                        }
                        newAdded.push_back(c);
                    }
                }
            }
        }
        toUpdate.insert(toUpdate.end(), newAdded.begin(), newAdded.end());
    }
    while(newAdded.size() > 0);

    m_updatedCurves = toUpdate;

    // Update curves
	BOOST_FOREACH(PassiveRef<Curve3D> curve, m_updatedCurves)
	{
//		if (bSkipUpdatedCurveInSolve && curve->geomType() == Geometry3D::eSpline3D)
//			continue;
        if(curve->geomType() == Geometry3D::eSpline3D)
        {
            SketchSolverSpline* solverSp = solverGeometry(curve)->solverGeom<SketchSolverSpline>(eSolverGeomSpline);
            Spline3D* sp = curve->query<Spline3D>();
            std::vector<AcGePoint3d> pts;
            std::vector<Ns::PassiveRef<Ns::Geometry::Point3D> > entities = sp->fitPointEntities();

            BOOST_FOREACH(Ns::PassiveRef<Ns::Geometry::Point3D> pt, entities)
            {
                pts.push_back(pt->point());
            }
            if(pts.size() > 1 && !sp->isFixed())
            {
                AcGeNurbCurve3d c = solverSp->computeSpline(pts, true);
                sp->setCurve(c);
            }
        }
        else
        {
            curve->updateCurve();
        }
	}

#ifdef _DEBUG
	checkSolverResults();
#endif

	return true;
}

void SketchConstraintSolverContext::addUpdatedPoint(PassiveRef<Point3D> pt)
{
	m_updatedPoints.push_back(pt);
}

void SketchConstraintSolverContext::dumpSolverData()
{
	std::vector<Ns::IString> strData;
	m_consData.dumpSolverData(strData);
}

bool SketchConstraintSolverContext::drag(AcGePoint3d& oldPos, const AcGePoint3d& newPos, const AcGeVector3d& viewVec)
{
	if (oldPos.isEqualTo(newPos))
	{
		// Skip solving, but need update solver geometries or it will revert to the initial state (due to transaction)
		updateSolverGeometries(false, false);
		return true;
	}

	bool res = m_pVCSInterface->prioritizedDrag(m_dragData.bodiesToDrag(), oldPos, newPos, viewVec);

	if (!oldPos.isEqualTo(newPos))
		oldPos = newPos;

	// Always updating geometries based on vcs bodies, update to the previous state if current fail in dragging.
	// It's to make the dragging not flip after user end dragging when there have solving failures during dragging.
	updateSolverGeometries(res);

#ifdef _DEBUG
	if (!res)
		dumpSolverData();
#endif	

	return res;
}

bool SketchConstraintSolverContext::dragBegin(PassiveRef<MSketch>& rSketch,
	const std::vector< PassiveRef<Geometry3D> >& geomtriesToDrag, 
	const AcGePoint3d& startPt)
{
	if (!initialize(rSketch, geomtriesToDrag))
		return false;

	// Prepare inputs
	m_dragData.resetData(startPt);

	// Add geometries for dragging
	std::vector<SketchSolverGeometry*> tempSolverGeomList;
	BOOST_FOREACH(PassiveRef<Geometry3D> rGeom, geomtriesToDrag)
	{
		SketchSolverGeometry* pGeom = solverGeometry(rGeom);
		m_dragData.addGeomToDrag(pGeom);
		tempSolverGeomList.push_back(pGeom);
	}

	// Add extra geometries for dragging
	BOOST_FOREACH(SketchSolverGeometry* pGeom, tempSolverGeomList)
	{
		pGeom->addExtraGeomsForDragging(m_dragData);
	}

	// Add extra handles for dragging
	BOOST_FOREACH(SketchSolverGeometry* pGeom, m_dragData.geometriesToDrag())
	{
		if (!pGeom->addHandlesForDragging(m_dragData))
			return false;
		m_consData.handleSymmetryConsWhenDragging(pGeom);
	}

	// Give solver geometry a chance to add extra logic
	GeomToSolverGeomMap::iterator iter = m_geomToSolverGeomMap.begin();
	for (; iter != m_geomToSolverGeomMap.end(); iter++)
	{
		SketchSolverGeometry* pGeom = iter->second;
		pGeom->addExtraHandlesForDragging(m_dragData);
	}

	// Do initial solve
	if (!m_pVCSInterface->solveForDragBegin())
	{
		// Current it doesn't work, need to find another way (zhanglo)
		return false;
		//m_consData.revertTempChanges(*this);
		//return m_pVCSInterface->solveForDragBegin();
	}
	return true;
}

void SketchConstraintSolverContext::getChangedGeometries(std::vector< PassiveRef<Geometry3D> >& gemosChanged)
{
	gemosChanged.clear();
	gemosChanged.assign(m_updatedPoints.begin(), m_updatedPoints.end());
	gemosChanged.insert(gemosChanged.end(), m_updatedCurves.begin(), m_updatedCurves.end());
}

bool SketchConstraintSolverContext::isDragging() const
{
	return m_bIsDragging;
}

SketchVCSStatus* SketchConstraintSolverContext::status()
{
	return m_pStatus;
}

bool SketchConstraintSolverContext::solveByCheckingUnmeetConstraints(ISketchConstraint* pCons)
{
	// Get un-meet constraints
	SketchToVCSConstraintMap unmeetConsMap;
	m_consData.getUnmeetConsHandle(unmeetConsMap);
	if (unmeetConsMap.empty())
	{
		if (pCons == NULL)
			return true;
		else
			return false;
	}

	if (NULL != pCons)
	{
		if (unmeetConsMap.size() == 1 && unmeetConsMap.find(pCons) != unmeetConsMap.end())
			return false;
	}

	// De-active all 
	BOOST_FOREACH(SketchToVCSConstraintMap::value_type& val, unmeetConsMap)
	{
		SolverGeomConsList& itemList = val.second;
		BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, itemList)
		{
			VCSConHandle* consH = pItem->consHandle();
			if (NULL == consH)
				continue;
			if (!pItem->isRelationMeet())
				consH->setActive(false);
		}
	}

	// Solve one by one
	BOOST_FOREACH(SketchToVCSConstraintMap::value_type& val, unmeetConsMap)
	{
		SolverGeomConsList& itemList = val.second;
		// active
		m_newAddedCons.clear();
		BOOST_FOREACH(SketchVCSConstraintDataItem* pItem, itemList)
		{
			VCSConHandle* consH = pItem->consHandle();
			if (NULL == consH)
				continue;
			consH->setActive(true);
			m_newAddedCons.push_back(consH);
		}
		// solve
		bool res = m_pVCSInterface->minMovementSolve(m_newAddedCons, val.first->constraintType(), false);
		NEUTRON_ASSERT(res);
		if (!res)
			return res;
	}

	return updateSolverGeometries(true);
}

bool SketchConstraintSolverContext::isRelationMet()
{
	// Check value before updating (current return false if circle/line is degenerated)
	GeomToSolverGeomMap::iterator iter = m_geomToSolverGeomMap.begin();
	for (; iter != m_geomToSolverGeomMap.end(); iter++)
	{
		SketchSolverGeometry* pGeom = iter->second;
		if (!pGeom->checkPreUpdateValue())
			return false;
	}
	return true;
}

double SketchConstraintSolverContext::tol() const
{
	return m_tol.equalPoint();
}

const AcGeTol& SketchConstraintSolverContext::tolEx() const
{
	return m_tol;
}
