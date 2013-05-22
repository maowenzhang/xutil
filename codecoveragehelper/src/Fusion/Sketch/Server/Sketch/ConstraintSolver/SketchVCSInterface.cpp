#include "Pch/SketchPch.h"
#include "SketchVCSInterface.h"
#include "SketchVCSConstraintData.h"
#include "SketchVCSExtCurve.h"
#include "SketchVCSExtRigidTester.h"
#include "SketchSolverPoint.h"
#include "SketchSolverLine.h"
#include "SketchSolverCircArc.h"
#include "SketchSolverEllipticArc.h"
#include "SketchSolverSpline.h"
#include "SketchVCSStatus.h"
#include "../Utils/SketchLogMgr.h"

#include <Server/Base/Assert/Assert.h>

#include <Server/Component/Assembly/VCSInterface.h>
#include <Server/Component/Assembly/VCSConv.h>
#include <gelnsg3d.h>
#include <Server/Geometry/Geometry3D/Geometry3D.h>

using namespace Ns::Comp;
using namespace SKs::Constraint;

SketchVCSInterface::SketchVCSInterface(SketchVCSStatus* pStatus)
	: m_pVCSInterface(VCSInterface::make()),
	m_horizontalVec(AcGeVector3d::kXAxis),
	m_verticalVec(AcGeVector3d::kYAxis),
	m_onPlaneDefault(AcGePlane::kXYPlane),
	m_pStatus(pStatus)
{
	NEUTRON_ASSERT(NULL != m_pStatus);

	m_pHorizontalGeomBody = createRBody();
	m_pHorizontalGeomBody->setGrounded(true);
	m_pVerticalGeomBody = createRBody();	
	m_pVerticalGeomBody->setGrounded(true);
	m_pXYPlaneBody = createRBody();
	m_pXYPlaneBody->setGrounded(true);

	// Use 1.0e-6, same as ASM used in sketch profile building
	m_tol.setEqualPoint(1.0e-6);
	m_tol.setEqualVector(1.0e-6);
	m_pVCSInterface->vcsSystem()->setTol(m_tol.equalPoint(), m_tol.equalPoint());
}

SketchVCSInterface::~SketchVCSInterface()
{
	VCSInterface::destroy(m_pVCSInterface);
}

VCSSystem* SketchVCSInterface::vcsSystem()
{
	return m_pVCSInterface->vcsSystem();
}

bool SketchVCSInterface::solve(unsigned int mode, bool bCacheBeforeSolve)
{
	VCSStatus status = m_pVCSInterface->solve(mode, bCacheBeforeSolve);
	return m_pStatus->isSuccess(status);
}

bool SketchVCSInterface::minMovementSolve(std::vector<VCSConHandle*>& newAddedCons, 
	SketchConstraintType consType,
	bool checkRedundancy)
{
	NEUTRON_ASSERT(!newAddedCons.empty());
	if (newAddedCons.empty())
		return false;

	VCSCollection excludedBodies;
	excludedBodies.add(m_pXYPlaneBody);

	int icount = static_cast<int>(newAddedCons.size());
	unsigned int mode = k2D;
	VCSConHandle* disturbedConH = NULL;
	VCSStatus status = kNull;
	if (icount == 1)
	{
		SKETCHLOG_TIME("vcsSystem()->minMovementSolve");
		status = m_pVCSInterface->vcsSystem()->minMovementSolve(
			newAddedCons[0], 
			excludedBodies, 
			mode, 
			checkRedundancy,
			&disturbedConH);
		return m_pStatus->isSuccess(status);
	}

	if (icount == 2)
	{
		if (SKs::eEqual == consType)
		{
			SKETCHLOG_TIME("vcsSystem()->minMovementSolveForEqualCons");
			status = m_pVCSInterface->vcsSystem()->minMovementSolveForEqualCons(newAddedCons[0], newAddedCons[1], excludedBodies, mode);
			return m_pStatus->isSuccess(status);
		}
		else if (SKs::eHorizontalDim == consType || SKs::eVerticalDim == consType)
		{
			SKETCHLOG_TIME("vcsSystem()->minMovementSolve");
			// Only need to solve first one
			status = m_pVCSInterface->vcsSystem()->minMovementSolve(newAddedCons[0], excludedBodies, mode);
			return m_pStatus->isSuccess(status);
		}
		else
		{
			newAddedCons[1]->setActive(false);
			status = m_pVCSInterface->vcsSystem()->minMovementSolve(newAddedCons[0], excludedBodies, mode);
			if (m_pStatus->isSuccess(status))
			{
				newAddedCons[1]->setActive(true);
				status = m_pVCSInterface->vcsSystem()->minMovementSolve(newAddedCons[1], excludedBodies, mode);
				if (m_pStatus->isSuccess(status))
					return true;
			}
			return false;
		}
	}

	// For constraint which has 3 items (for lineMidPoint)
	if (icount == 4)
	{
		SKETCHLOG_TIME("vcsSystem()->minMovementSolve");
		// solve 4th one
		newAddedCons[3]->setActive(true);
		status = m_pVCSInterface->vcsSystem()->minMovementSolve(newAddedCons[3], excludedBodies, mode, 
			checkRedundancy,
			&disturbedConH);
		return m_pStatus->isSuccess(status);
	}

	NEUTRON_ASSERT(false);
	return false;
}

bool SketchVCSInterface::solveForDragBegin()
{
	SKETCHLOG_TIME("SketchVCSInterface::solveForDragBegin");

	// Do an initialization solve when drag start, use the “kNeedDOF | kNoSetMerging”  mode for now.
	// Need “kNeedDOF” because we need a correct DOF of the dragging body when entering the prioritized 
	// drag function. VCS’ll move the dragging body according to its DOF. The VCSSystem::prioritizedDrag() 
	// assume user has done an initialization solve.
	VCSStatus status = m_pVCSInterface->solve(k2D | kNeedDOF | kNoSetMerging | kLargeNumeric, false);
	if (m_pStatus->isSuccess(status))
		return true;
	//NEUTRON_ASSERT(false);
	return false;
}

bool SketchVCSInterface::prioritizedDrag(const VCSCollection& bodies,
	AcGePoint3d& oldPos,
	const AcGePoint3d& newPos,
	const  AcGeVector3d& viewVec)
{
	SKETCHLOG_TIME("SketchVCSInterface::prioritizedDrag");
	// For debugging
#ifdef _DEBUG
	AcGePoint3d tempOldPos = oldPos;
#endif	
	VCSSolveMode mode = k2D;
	VCSStatus status = m_pVCSInterface->prioritizedDrag(bodies, oldPos, newPos, viewVec, mode);
	return m_pStatus->isSuccess(status);
}

bool SketchVCSInterface::fixConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1())
		return false;

	data.geom1()->vcsBody()->setGrounded(true);
	return true;
}

bool SketchVCSInterface::onPlaneConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1())
		return false;

	SketchSolverGeometry* pGeom1 = data.geom1();
	ESolverGeometryType geom1Type = data.geom1()->geomType();
	if (eSolverGeomPoint == geom1Type)
	{
		VCSConHandle* pConHandle = NULL;
		AcGePoint3d geBiasPt;
		m_pVCSInterface->distPtPl(pConHandle,
			data.geom1()->position(),
			m_onPlaneDefault,
			0.0,
			data.geom1()->vcsBody(),
			m_pXYPlaneBody,
			NULL, NULL,
			false, geBiasPt, kVCSEQ);
		data.addConsHandle(pConHandle);
		return true;
	}
	if (eSolverGeomLine == geom1Type)
	{
		// Do nothing without add redundant constraints to improve the redundancy check
		return true;
	}
	if (eSolverGeomCirArc == geom1Type)
	{
		VCSConHandle* pConHandle = NULL;
		angleConstraintEx(pConHandle,
			data.geom1()->vcsBody(),
			m_pXYPlaneBody,
			data.geom1()->circArc().normal(),
			m_onPlaneDefault.normal(),
			0.0);
		data.addConsHandle(pConHandle);
		return true;
	}
	if (eSolverGeomEllipticArc == geom1Type)
	{
		VCSConHandle* pConHandle = NULL;
		angleConstraintEx(pConHandle,
			data.geom1()->vcsBody(),
			m_pXYPlaneBody,
			data.geom1()->ellipArc().normal(),
			m_onPlaneDefault.normal(),
			0.0);
		data.addConsHandle(pConHandle);
		return true;
	}
	
	if (eSolverGeomSpline == geom1Type)
	{
		// do nothing for spline as all its points are constrained on the plane
		return true;
	}

	return false;
}

bool SketchVCSInterface::coincidentConstraintPtPt(SketchVCSConstraintData& data,
	const AcGePoint3d& point1, const AcGePoint3d& point2)
{
	if (!data.hasGeom1And2())
		return false;

	VCSConHandle* pConHandle = NULL;
	m_pVCSInterface->distPtPt(pConHandle, 
		point1, 
		point2,
		0.0, 
		data.geom1()->vcsBody(),
		data.geom2()->vcsBody(),
		NULL, NULL, kVCSEQ);
	data.addConsHandle(pConHandle);
	return true;
}

bool SketchVCSInterface::coincidentConstraint(SketchVCSConstraintData& data)
{
	data.consValue(0.0);
	return distanceConstraint(data, NULL);
}

bool SketchVCSInterface::distanceConstraint(SketchVCSConstraintData& data, VCSVariable* pVariable)
{
	if (!data.hasGeom1And2())
		return false;

	// Point coincident
	if (data.checkGeomTypes(eSolverGeomPoint))
	{
		VCSConEqMode eqMode = kVCSEQ;
		if (data.curConsItem().isMinimumValueCons())
			eqMode = kVCSGE;

		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->distPtPt(pConHandle, 
			data.geom1()->position(),
			data.geom2()->position(),
			data.consValue(), 
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			NULL, NULL, eqMode, pVariable);	
		
		bool isRelationMeet = data.geom1()->position().isEqualTo(data.geom2()->position(), m_tol);
		data.addConsHandle(pConHandle, isRelationMeet);
		return true;
	}
	// Point on line
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomLine))
	{
		AcGePoint3d geBiasPt = data.curConsItem().biasPt();
		bool bUseBiasPt = false;
		if (!geBiasPt.isEqualTo(AcGePoint3d::kOrigin))
			bUseBiasPt = true;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->distPtLn(pConHandle, 
			data.geom1()->position(),
			data.geom2()->line(),
			data.consValue(), 
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			NULL, NULL,
			bUseBiasPt, geBiasPt,
			kVCSEQ);

		double dist = data.geom2()->line().distanceTo(data.geom1()->position(), m_tol);
		bool isRelationMeet = fabs(dist - data.consValue()) < m_tol.equalPoint();

		data.addConsHandle(pConHandle, isRelationMeet);

		return true;
	}
	// Line to line
	if (data.checkGeomTypes(eSolverGeomLine, eSolverGeomLine))
	{
		AcGePoint3d geBiasPt;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->distLnLn(pConHandle, 
			data.geom1()->line(),
			data.geom2()->line(),
			data.consValue(),
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			NULL, NULL,
			false, geBiasPt, geBiasPt,
			kVCSEQ);

		double dist = data.geom1()->line().distanceTo(data.geom2()->line());
		bool isRelationMeet = fabs(dist - data.consValue()) < m_tol.equalPoint();
		data.addConsHandle(pConHandle, isRelationMeet);
		return true;
	}

	// Point on CircleArc
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomCirArc))
	{
		if (data.consValue() <= AcGeContext::gTol.equalPoint())
		{
			SketchSolverCircArc* pCircArc = data.geom2()->solverGeom<SketchSolverCircArc>(eSolverGeomCirArc);
			if (NULL == pCircArc)
				return false;

			return circleMatePtCir(data, data.geom1()->position(), data.geom1()->vcsBody(), pCircArc);
		}
		else
		{
			NEUTRON_ASSERT(false && _DNGH("Not supported!"));
		}
	}

	// Point on spline
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomSpline))
		//|| data.checkGeomTypes(eSolverGeomPoint, eSolverGeomEllipticArc))
	{
		if (data.consValue() > AcGeContext::gTol.equalPoint())
		{
			NEUTRON_ASSERT(false && _DNGH("Not supported!"));
			return false;
		}
		if (data.geom2()->geomType() == eSolverGeomSpline)
		{
			SketchSolverSpline* pSp = data.geom2()->solverGeom<SketchSolverSpline>(eSolverGeomSpline);
			if (pSp)
				pSp->hasVCSConstraint(true);
		}
		VCSMPoint3d biasPt = GeConv::toVCSMPoint3d(data.geom1()->position());
		VCSMPoint3d ptOnBody1 = biasPt;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->vcsSystem()->create3dMatePtCur(pConHandle,
			biasPt,			
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			ptOnBody1,
			data.geom2()->extCurve(),
			NULL, data.geom2()->extGeometry());
		data.addConsHandle(pConHandle);
		return true;
	}

	// Point on EllipticArc
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomEllipticArc))
	{
		if (data.consValue() <= AcGeContext::gTol.equalPoint())
		{
			SketchSolverEllipticArc* pEllipseArc = data.geom2()->solverGeom<SketchSolverEllipticArc>(eSolverGeomEllipticArc);
			if (NULL == pEllipseArc)
				return false;

			return matePtEllipse(data, data.geom1()->position(), data.geom1()->vcsBody(), pEllipseArc);
		}
		else
		{
			NEUTRON_ASSERT(false && _DNGH("Not supported!"));
		}
	}

	return false;
}

bool SketchVCSInterface::angleConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1And2())
		return false;

	if (data.checkGeomTypes(eSolverGeomLine))
	{
		// Skip if the geometries are degenerated (such as line turns as point)
		if (data.geom1()->degenerate() || data.geom2()->degenerate())
			return true;		

		VCSConHandle* pConHandle = NULL;
		bool res = angleConstraintEx(pConHandle,
			data.geom1()->vcsBody(), 
			data.geom2()->vcsBody(), 
			data.geom1()->line().direction(), 
			data.geom2()->line().direction(),
			data.consValue());
		data.addConsHandle(pConHandle);
		// Add reactor for sketchSolverLine when needed (has angle type constraint)
		data.geom1()->addReactor();
		data.geom2()->addReactor();
		return res;
	}

	return false;
}

bool SketchVCSInterface::angleConstraintEx(VCSConHandle*& pConHandle, VCSRigidBody* pBody1, VCSRigidBody* pBody2, 
	const AcGeVector3d& vector1, const AcGeVector3d& vector2, double dAngle)
{
	VCSRigidBody* refBody = NULL;
	AcGeVector3d refVector;
	// Always use 'kReference' ... this gives "right hand rule from 1 to 2" 
	VCSVecSense vcsSense = kReference;
	if (dAngle < 10e-6)
	{
		// we shouldn't create ref body when use kParallel, or vcs will change to kReference internally
		vcsSense = kParallel; // both kCodirectional and kOpposed
	}
	else
	{
		// we have to set ref body when use "kReference"
		refBody =  createRBody();
		refVector = vector1.crossProduct(vector2);
	}

	AcGePoint3d biasPt;
	m_pVCSInterface->angVecVec(pConHandle,
		vector1, vector2, 
		vcsSense, dAngle, 
		pBody1,
		pBody2,
		NULL, NULL,
		false, biasPt, biasPt, 
		refBody, refVector, NULL,
		kVCSEQ, NULL);
	
	double angle = vector1.angleTo(vector2, refVector);
	double dist = angle - dAngle;

	return true;
}

bool SketchVCSInterface::parallelConstraint(SketchVCSConstraintData& data)
{
	data.consValue(0.0);
	return angleConstraint(data);
}

bool SketchVCSInterface::perpendicularConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1And2())
		return false;

	// Line and Line
	if (data.checkGeomTypes(eSolverGeomLine))
	{
		data.consValue(Ns::kHalfPi);
		return angleConstraint(data);
	}

	// Circle and Line, constraint circle's center on the line
	if (data.checkGeomTypes(eSolverGeomLine, eSolverGeomCirArc))
	{
		SketchSolverGeometry* pCenter = data.geom2()->centerPointSolverGeometry();
		if (NULL == pCenter)
			return false;

		data.resetDataEX(pCenter, data.geom1());
		return coincidentConstraint(data);
	}

	return false;
}

bool SketchVCSInterface::concentricConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1And2())
		return false;

	// Circle and Ellipse
	// Also include circle-circle as conCirCir has some issues 
	// (Failed to drag when has concentric on projected circle-circle FUS-3711)
	if (data.checkGeomTypes(eSolverGeomCirArc, eSolverGeomEllipticArc)
		|| data.checkGeomTypes(eSolverGeomEllipticArc, eSolverGeomEllipticArc)
		|| data.checkGeomTypes(eSolverGeomCirArc))
	{
		SketchSolverGeometry* pCenter1 = data.geom1()->centerPointSolverGeometry();
		if (NULL == pCenter1)
			return false;
		SketchSolverGeometry* pCenter2 = data.geom2()->centerPointSolverGeometry();
		if (NULL == pCenter2)
			return false;
		
		data.resetDataEX(pCenter1, pCenter2);
		bool isRelationMeet = pCenter1->position().isEqualTo(pCenter2->position(), m_tol);
		return coincidentConstraint(data);
	}
	// Point and Ellipse
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomEllipticArc))
	{
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->distPtPt(pConHandle, 
			data.geom1()->position(), 
			data.geom1()->position(),
			0.0, 
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			NULL, data.geom2()->extGeometry(), kVCSEQ);
		data.addConsHandle(pConHandle);
		return true;
	}

	return false;
}

bool SketchVCSInterface::collinearConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1And2())
		return false;

	// Line and Line
	if (data.checkGeomTypes(eSolverGeomLine))
	{
		VCSConHandle* pConHandle = NULL;
		AcGePoint3d biasPt;
        
        AcGeLine3d line1 = data.geom1()->line();
        AcGeLine3d line2 = data.geom2()->line();

		m_pVCSInterface->distLnLn(pConHandle,
			line1,
			line2,
			0.0,
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			NULL, NULL,
			false, biasPt, biasPt, kVCSEQ);
		
		bool isRelationMeet = line1.isColinearTo(line2, m_tol);
		data.addConsHandle(pConHandle, isRelationMeet);
		return true;
	}

	return false;
}

bool SketchVCSInterface::tangentConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1And2())
		return false;

	// Line and Circle
	if (data.checkGeomTypes(eSolverGeomLine, eSolverGeomCirArc))
	{
		SketchSolverCircArc* pCircArc = data.geom2()->solverGeom<SketchSolverCircArc>(eSolverGeomCirArc);
		if (NULL == pCircArc)
			return false;

		VCSConHandle* pConHandle = NULL;
		AcGePoint3d pntOnOtherCrv;
		AcGePoint3d biasPt = data.geom1()->line().closestPointTo(data.geom2()->circArc(), pntOnOtherCrv);
		VCSVecSense sense = kCodirectional;

		m_pVCSInterface->tanLnCir(pConHandle,
			data.geom1()->line(),
			0.0,
			sense,
			data.geom1()->vcsBody(),
			pCircArc->variableCircleHandle(),
			NULL, NULL, 
			true, biasPt, kVCSEQ);

		double dist = pCircArc->circArc().distanceTo(data.geom1()->line(), m_tol);
		bool isRelationMeet = fabs(dist) < m_tol.equalPoint(); 

		data.addConsHandle(pConHandle, isRelationMeet);
		return true;
	}

	// Circle and Circle
	if (data.checkGeomTypes(eSolverGeomCirArc))
	{
		SketchSolverCircArc* pCircArc1 = data.geom1()->solverGeom<SketchSolverCircArc>(eSolverGeomCirArc);
		if (NULL == pCircArc1)
			return false;
		SketchSolverCircArc* pCircArc2 = data.geom2()->solverGeom<SketchSolverCircArc>(eSolverGeomCirArc);
		if (NULL == pCircArc2)
			return false;

		// Check whether we should use outside tangent/inside tangent
		bool bOutSideTangent = true;
		const AcGeCircArc3d& arc1 = pCircArc1->circArc();
		const AcGeCircArc3d& arc2 = pCircArc2->circArc();
		if (arc1.isInside(arc2.center()) || arc2.isInside(arc1.center()))
		{
			bOutSideTangent = false;
		}

		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->tanCirCir(pConHandle,
			pCircArc1->variableCircleHandle(),
			pCircArc2->variableCircleHandle(),
			NULL, NULL, bOutSideTangent, 
			0.0, kVCSEQ);
		
		double dist = arc1.distanceTo(arc2);
		bool isRelationMeet = fabs(dist) < m_tol.equalPoint();

		data.addConsHandle(pConHandle, isRelationMeet);
		return true;
	}

	// Line and Ellipse
	if (data.checkGeomTypes(eSolverGeomLine, eSolverGeomEllipticArc))
	{
		AcGePoint3d pntOnOtherCrv;
		AcGePoint3d biasPt = data.geom1()->line().closestPointTo(data.geom2()->ellipArc(), pntOnOtherCrv);

		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->vcsSystem()->create3dTanLnEll(pConHandle,
			true,
			GeConv::toVCSMPoint3d(biasPt),
			data.geom1()->vcsBody(),
			data.geom1()->lineEx(),
			data.geom2()->varGeomHandle(),
			NULL, NULL);

		data.addConsHandle(pConHandle);
		return true;
	}

	// Ellipse and Ellipse
	if (data.checkGeomTypes(eSolverGeomEllipticArc, eSolverGeomEllipticArc))
	{
		VCSMPoint3d biasPt;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->vcsSystem()->create3dTanEllEll(pConHandle,
			false, biasPt, biasPt,
			data.geom1()->varGeomHandle(),
			data.geom2()->varGeomHandle(),
			NULL, NULL);

		data.addConsHandle(pConHandle);
		return true;
	}

	// Circle and Ellipse
	if (data.checkGeomTypes(eSolverGeomCirArc, eSolverGeomEllipticArc))
	{
		VCSMPoint3d biasPt;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->vcsSystem()->create3dTanCirEll(pConHandle,
			false, biasPt, biasPt,
			data.geom1()->varGeomHandle(),
			data.geom2()->varGeomHandle(),
			NULL, NULL);

		data.addConsHandle(pConHandle);
		return true;
	}

	// Line tangent with spline
	if (data.checkGeomTypes(eSolverGeomLine, eSolverGeomSpline))
	{
		// if the spline is fixed, we still use VCS to solve the connected geometry
        if(!data.geom2()->geometry()->isFixed())
        {
            return true;
        }
		if (data.geom2()->geomType() == eSolverGeomSpline)
		{
			SketchSolverSpline* pSp = data.geom2()->solverGeom<SketchSolverSpline>(eSolverGeomSpline);
			if (pSp)
				pSp->hasVCSConstraint(true);
		}
		VCSMLine3d line = GeConv::toVCSMLine3d(data.geom1()->line());
		VCSMPoint3d bias1 = line.pointOnLine();
		VCSMPoint3d bias2 = bias1;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->vcsSystem()->create3dTanLnCur(pConHandle,
			bias1,
			bias2,
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			line,
			data.geom2()->extCurve(),
			NULL, NULL);
		
		data.addConsHandle(pConHandle);
		return true;
	}
	
	// Circle tangent with spline
	if (data.checkGeomTypes(eSolverGeomCirArc, eSolverGeomSpline))
	{
		// if the spline is fixed, we still use VCS to solve the connected geometry
        if(!data.geom2()->geometry()->isFixed())
        {
            return true;
        }
		if (data.geom2()->geomType() == eSolverGeomSpline)
		{
			SketchSolverSpline* pSp = data.geom2()->solverGeom<SketchSolverSpline>(eSolverGeomSpline);
			if (pSp)
				pSp->hasVCSConstraint(true);
		}
		VCSMPoint3d biasPt;
		VCSVecSense sense = kParallel;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->vcsSystem()->create3dTanCirCur(pConHandle,
			biasPt,
			data.geom2()->vcsBody(),
			data.geom1()->varGeomHandle(),
			sense,
			data.geom2()->extCurve(),
			NULL, NULL);
		data.addConsHandle(pConHandle);
		return true;
	}

	// Spline and Spline
    if (data.checkGeomTypes(eSolverGeomSpline, eSolverGeomSpline))
	{
        /*
		VCSMPoint3d biasPt;
		VCSVecSense sense = kParallel;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->vcsSystem()->create3dTanCurCur(pConHandle,
			biasPt,
            biasPt,
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			data.geom1()->extCurve(),
			data.geom2()->extCurve(),
			NULL, NULL);
		data.addConsHandle(pConHandle);
        */
		return true;
	}
	return false;
}

bool SketchVCSInterface::horizontalConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1())
		return false;

	ESolverGeometryType geom1Type = data.geom1()->geomType();
	if (geom1Type == eSolverGeomLine)
	{
		VCSConHandle* pConHandle = NULL;
		bool res = angleConstraintEx(pConHandle,
			data.geom1()->vcsBody(),
			m_pHorizontalGeomBody,
			data.geom1()->lineDirection(),
			m_horizontalVec,
			0.0);
		
		bool isRelationMeet = data.geom1()->lineDirection().isParallelTo(m_horizontalVec, m_tol);
		data.addConsHandle(pConHandle, isRelationMeet);

		// Add reactor for sketchSolverLine when needed (has angle type constraint)
		data.geom1()->addReactor();
		return res;
	}

	// For point point horizontal
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomPoint))
	{
		SketchSolverPoint* pPoint1 = data.geom1()->solverGeom<SketchSolverPoint>(eSolverGeomPoint);

		VCSRigidBody* pConsBody = pPoint1->getConstructBodyHorizontal();

		// 2nd point on line (reuse 1pt body)
		AcGeLine3d geTempLine = AcGeLine3d(data.geom1()->position(), m_horizontalVec);
		VCSConHandle* pConHandle = NULL;
		AcGePoint3d biasPt;		
		m_pVCSInterface->distPtLn(pConHandle, 
			data.geom2()->position(),
			geTempLine,
			0.0,
			data.geom2()->vcsBody(),
			pConsBody,
			NULL, NULL,
			false, biasPt,
			kVCSEQ);
		data.addConsHandle(pConHandle);
		return true;
	}

	return false;
}

bool SketchVCSInterface::verticalConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1())
		return false;

	ESolverGeometryType geom1Type = data.geom1()->geomType();
	if (geom1Type == eSolverGeomLine)
	{
		VCSConHandle* pConHandle = NULL;
		bool res = angleConstraintEx(pConHandle, 
			data.geom1()->vcsBody(),
			m_pVerticalGeomBody,
			data.geom1()->lineDirection(),
			m_verticalVec,
			0.0);

		bool isRelationMeet = data.geom1()->lineDirection().isParallelTo(m_verticalVec, m_tol);
		data.addConsHandle(pConHandle, isRelationMeet);

		// Add reactor for sketchSolverLine when needed (has angle type constraint)
		data.geom1()->addReactor();
		return res;
	}

	// For point point horizontal
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomPoint))
	{
		// Add constraint to make sure the line is vertical/horizontal
		AcGeLine3d geTempLine = AcGeLine3d(data.geom1()->position(), m_verticalVec);
		VCSConHandle* pConHandle = NULL;
		bool res = angleConstraintEx(pConHandle, 
			data.geom1()->vcsBody(),
			m_pVerticalGeomBody,
			geTempLine.direction(),
			m_verticalVec,
			0.0);

		// 2nd point on line (reuse 1pt body)
		AcGePoint3d biasPt;		
		m_pVCSInterface->distPtLn(pConHandle, 
			data.geom2()->position(),
			geTempLine,
			0.0,
			data.geom2()->vcsBody(),
			data.geom1()->vcsBody(),
			NULL, NULL,
			false, biasPt,
			kVCSEQ);
		data.addConsHandle(pConHandle);
		return res;
	}

	return false;
}

VCSRigidBody* SketchVCSInterface::createRBody()
{
	int id;
	AcGeMatrix3d transform;
	bool bGrounded = false;
	VCSRigidBody* pBody = m_pVCSInterface->createRBody(transform, bGrounded, id);
	NEUTRON_ASSERT(NULL != pBody);
	return pBody;
}

bool SketchVCSInterface::equalConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1And2())
		return false;

	// Line and Line
	if (data.checkGeomTypes(eSolverGeomLine))
	{
		SketchSolverGeometry* pGeom1 = data.geom1();
		SketchSolverGeometry* pGeom2 = data.geom2();

		SketchSolverLine* pLine1 = pGeom1->solverGeom<SketchSolverLine>(eSolverGeomLine);
		if (NULL == pLine1)
			return false;

		// Create variables with equation
		double lineLength = pLine1->length();
		VCSVariable* pV1 = m_pVCSInterface->createVariable(lineLength);
		if (!pV1)
			return false;

		// Add constraint with the variables
		data.resetDataEX(pGeom1, NULL, 0.0, true);
		if (!lineLengthConstraint(data, pV1))
			return false;

		data.resetDataEX(pGeom2, NULL, 0.0, true);
		return lineLengthConstraint(data, pV1);
	}

	// Circle and Circle
	if (data.checkGeomTypes(eSolverGeomCirArc))
	{
		SketchSolverGeometry* pGeom1 = data.geom1();
		SketchSolverGeometry* pGeom2 = data.geom2();

		// Create variables with equation
		double radius = pGeom1->circArc().radius();
		VCSVariable* pV1 = m_pVCSInterface->createVariable(radius);
		if (!pV1)
			return false;
		data.resetDataEX(pGeom1, NULL);
		if (!circleRadiusConstraint(data, pV1))
			return false;
		data.resetDataEX(pGeom2, NULL);
		return circleRadiusConstraint(data, pV1);
	}

	// TODO: ellipse and spline
	NEUTRON_ASSERT(false && _DNGH("Not supported!"));
	return false;
}

bool SketchVCSInterface::lineLengthConstraint(SketchVCSConstraintData& data, VCSVariable* pVariable)
{
	if (!data.hasGeom1())
		return false;

	if (data.geom1()->useCombinedBody())
		return true;

	ESolverGeometryType geom1Type = data.geom1()->geomType();
	if (eSolverGeomLine == geom1Type)
	{
		SketchSolverLine* pLine1 = data.geom1()->solverGeom<SketchSolverLine>(eSolverGeomLine);
		if (NULL == pLine1)
			return false;

		SketchSolverGeometry* pStartPt = pLine1->solverGeomStartPt();
		SketchSolverGeometry* pEndPt = pLine1->solverGeomEndPt();
		
		data.resetDataEX(pStartPt, pEndPt, data.consValue());
		return distanceConstraint(data, pVariable);
	}

	NEUTRON_ASSERT(false && _DNGH("Not supported!"));
	return false;
}

bool SketchVCSInterface::linearDimensionConstraint(SketchVCSConstraintData& data)
{
	// One Geometry
	if (data.geomCount() == 1)
		return lineLengthConstraint(data);

	// Two Geometries: point to line, line to line, point to point
	if (data.geomCount() == 2)
	{
		data.preferMovingLaterCreatedGeom();
		return distanceConstraint(data);
	}

	NEUTRON_ASSERT(_DNGH("Not supported"));
	return false;
}

bool SketchVCSInterface::alignAxisDimensionConstraint(SketchVCSConstraintData& data, const AcGeVector3d& aligAxis)
{
	// One Geometry (line)
	if (data.geomCount() == 1)
	{
		if (eSolverGeomLine != data.geom1()->geomType())
			return false;		
		// Get two points
		SketchSolverLine* pLine1 = data.geom1()->solverGeom<SketchSolverLine>(eSolverGeomLine);
		if (NULL == pLine1)
			return false;
		SketchSolverGeometry* pStartPt = pLine1->solverGeomStartPt();
		SketchSolverGeometry* pEndPt = pLine1->solverGeomEndPt();

		data.resetDataEX(pStartPt, pEndPt, data.consValue());
		return alignAxisDimensionConstraint(data, aligAxis);
	}

	// Two Geometries: point to point
	if (data.checkGeomTypes(eSolverGeomPoint))
	{
		// Get one point as "aligned axis"
		AcGeLine3d geAlignedAxis = AcGeLine3d(data.geom2()->position(), aligAxis);

		// Create constraint
		AcGePoint3d biasPt;
		bool bUseBiasPt = false;
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->distPtLn(pConHandle, 
			data.geom1()->position(),
			geAlignedAxis,
			data.consValue(), 
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			NULL, NULL,
			bUseBiasPt, biasPt,
			kVCSEQ);
		data.addConsHandle(pConHandle);

		// Add extra constraint to make sure the align axis is vertical/horizontal
		if (aligAxis.isParallelTo(AcGeVector3d::kYAxis))
		{
			bool res = angleConstraintEx(pConHandle, 
				data.geom2()->vcsBody(),
				m_pVerticalGeomBody,
				geAlignedAxis.direction(),
				m_verticalVec,
				0.0);
		}
		else
		{
			bool res = angleConstraintEx(pConHandle, 
				data.geom2()->vcsBody(),
				m_pHorizontalGeomBody,
				geAlignedAxis.direction(),
				m_horizontalVec,
				0.0);
		}

		data.addConsHandle(pConHandle);		
		return true;
	}

	return false;
}

bool SketchVCSInterface::circleMatePtCir(SketchVCSConstraintData& data,
	AcGePoint3d ptOnCircle,
	VCSRigidBody* pPointOnCircleBody,
	SketchSolverCircArc* pCircArc)
{
	AcGePoint3d geBiasPt;
	VCSConHandle* pConHandle = NULL;

	bool bres = m_pVCSInterface->matePtCir(pConHandle,
		ptOnCircle,
		pPointOnCircleBody,
		pCircArc->variableCircleHandle(),
		NULL, NULL, false, geBiasPt);

	NEUTRON_ASSERT(bres);
	bool isRelationMeet = pCircArc->circArc().isOn(ptOnCircle, m_tol);
	data.addConsHandle(pConHandle, isRelationMeet);

	return bres;
}

bool SketchVCSInterface::matePtEllipse(SketchVCSConstraintData& data,
	AcGePoint3d ptOnEllipse,
	VCSRigidBody* pPointOnEllipseBody,
	SketchSolverEllipticArc* pEllipArc,
	bool useBias,
	AcGePoint3d geBiasPt,
	bool isFixed)
{
	VCSConHandle* pConHandle = NULL;

	bool bres = m_pVCSInterface->vcsSystem()->create3dMatePtEll(pConHandle,
		useBias,
		GeConv::toVCSMPoint3d(geBiasPt),
		pPointOnEllipseBody,
		GeConv::toVCSMPoint3d(ptOnEllipse),
		pEllipArc->variableEllipseHandle(),
		isFixed,
		NULL, NULL);
	NEUTRON_ASSERT(bres);

	bool isRelationMeet = pEllipArc->ellipArc().isOn(ptOnEllipse, m_tol);
	data.addConsHandle(pConHandle, isRelationMeet);

	return bres;
}

bool SketchVCSInterface::circleRadiusConstraint(SketchVCSConstraintData& data, VCSVariable* pVariable)
{
	if (!data.hasGeom1())
		return false;

	ESolverGeometryType geom1Type = data.geom1()->geomType();
	if (eSolverGeomCirArc == geom1Type)
	{
		SketchSolverCircArc* pCircArc = data.geom1()->solverGeom<SketchSolverCircArc>(eSolverGeomCirArc);
		if (NULL == pCircArc)
			return false;
		if (NULL != pVariable)
			pCircArc->hasVariableRadiusCons(true);

		VCSConEqMode eqMode = kVCSEQ;
		if (data.curConsItem().isMinimumValueCons())
			eqMode = kVCSGE;

		// Set circle as invariable if a circle has a dimension & dimension is not changed
		// This could improve the solving performance and dragging behavior
		if (pVariable == NULL && eqMode == kVCSEQ)
		{
			// Still add if it's new added constraint, as we need to input it to call minMovementSolve API and check over constraint
			if (!data.isAddingNewConstraint())
			{
				bool isRelationMeet = std::fabs(pCircArc->circArc().radius() - data.consValue()) < m_tol.equalPoint();
				NEUTRON_ASSERT(isRelationMeet);
				if (isRelationMeet)
				{
					pCircArc->varGeomHandle()->setInvariable(true);
					return true;
				}
			}
		}

		// Radius constraint
		VCSConHandle* pConHandle2 = NULL;
		bool bres = m_pVCSInterface->distPtPt(pConHandle2, 
			pCircArc->ptOnCircle(), 
			pCircArc->circArc().center(),
			data.consValue(), 
			pCircArc->vcsBodyPtOnCircle(data),
			pCircArc->vcsBody(),
			NULL, NULL, eqMode, pVariable);
		data.addConsHandle(pConHandle2);
		return bres;
	}

	NEUTRON_ASSERT(false && _DNGH("Not supported!"));
	return false;
}

bool SketchVCSInterface::ellipseRadiusConstraint(SketchVCSConstraintData& data, VCSVariable* pVariable, bool bMajorRadius)
{
	if (!data.hasGeom1())
		return false;

	ESolverGeometryType geom1Type = data.geom1()->geomType();
	if (eSolverGeomEllipticArc == geom1Type)
	{
		SketchSolverEllipticArc* pEllipse = data.geom1()->solverGeom<SketchSolverEllipticArc>(eSolverGeomEllipticArc);
		if (NULL == pEllipse)
			return false;

		VCSConEqMode eqMode = kVCSEQ;
		if (data.curConsItem().isMinimumValueCons())
			eqMode = kVCSGE;

		// Radius constraint
		bool bres = false;
		if (bMajorRadius)
		{
			VCSConHandle* pConHandle = NULL;
			bres = m_pVCSInterface->distPtPt(pConHandle, 
				pEllipse->getEllipMajorPt(),
				pEllipse->ellipArc().center(),
				data.consValue(), 
				pEllipse->internalBodyOnEllipMajor(data),
				pEllipse->vcsBody(),
				NULL, NULL, eqMode, pVariable);
			data.addConsHandle(pConHandle);
		}
		else
		{
			VCSConHandle* pConHandle = NULL;
			bres = m_pVCSInterface->distPtPt(pConHandle, 
				pEllipse->getEllipMinorPt(),
				pEllipse->ellipArc().center(),
				data.consValue(), 
				pEllipse->internalBodyOnEllipMinor(data),
				pEllipse->vcsBody(),
				NULL, NULL, eqMode, pVariable);
			data.addConsHandle(pConHandle);
		}	
		return bres;
	}

	NEUTRON_ASSERT(false && _DNGH("Not supported!"));
	return false;
}

bool SketchVCSInterface::circleDiameterConstraint(SketchVCSConstraintData& data)
{
	double dDiameter = data.consValue() / 2.0;
	data.consValue(dDiameter);
	return circleRadiusConstraint(data);
}

bool SketchVCSInterface::midPointConstraint(SketchVCSConstraintData& data)
{
	if (!data.hasGeom1And2())
		return false;

	// Point on middle of line
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomLine))
	{
		SketchSolverGeometry* pGeom1 = data.geom1();
		SketchSolverGeometry* pGeom2 = data.geom2();

		SketchSolverLine* pLine2 = pGeom2->solverGeom<SketchSolverLine>(eSolverGeomLine);
		if (pLine2 == NULL)
			return false;

		VCSRigidBody* midPointBody = pLine2->getMidPointBody();

		// constraint two points
		data.resetDataEX(pGeom1, pGeom2);
		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->distPtPt(pConHandle, 
			pGeom1->position(), 
			pGeom2->position(),
			0.0, 
			data.geom1()->vcsBody(),
			midPointBody,
			NULL, NULL, kVCSEQ);

		bool isRelationMeet = pGeom1->position().isEqualTo(pGeom2->position(), m_tol);

		data.addConsHandle(pConHandle, isRelationMeet);
		return true;
	}

	// Point on middle of arc
	if (data.checkGeomTypes(eSolverGeomPoint, eSolverGeomCirArc))
	{
		SketchSolverCircArc* pArc = data.geom2()->solverGeom<SketchSolverCircArc>(eSolverGeomCirArc);
		if (NULL == pArc->startPoint())
			return false; // only support arc not circle

		// #. Get construction body (a symmetry line between arc's end points)
		VCSRigidBody* pSymLineBody = createRBody();
		AcGeLineSeg3d endptsLineSeg(pArc->startPoint()->position(), pArc->endPoint()->position());
		AcGeLine3d geSymLine(endptsLineSeg.midPoint(), endptsLineSeg.direction().perpVector());
		AcGeLine3d geEndPtsLine(endptsLineSeg.startPoint(), endptsLineSeg.endPoint());
		VCSMLine3d symLine = GeConv::toVCSMLine3d(geSymLine);

		VCSConHandle* pConHandle = NULL;
		m_pVCSInterface->vcsSystem()->create3dSymmPtPtLn(pConHandle,
			pArc->startPoint()->vcsBody(),
			pArc->endPoint()->vcsBody(),
			pArc->startPoint()->positionEx(),
			pArc->endPoint()->positionEx(),
			pSymLineBody,
			symLine);
		//data.addConsHandle(pConHandle);

		// #. Get 2nd construction body (line on two end points)
		VCSRigidBody* pEndPtsLineBody = createRBody();
		VCSMLine3d endPtsLine = GeConv::toVCSMLine3d(geEndPtsLine);
		AcGePoint3d gebiasPt;
		m_pVCSInterface->distPtLn(pConHandle, 
			pArc->startPoint()->position(),
			geEndPtsLine,
			0.0, 
			pArc->startPoint()->vcsBody(),
			pEndPtsLineBody,
			NULL, NULL,
			false, gebiasPt,
			kVCSEQ);
		m_pVCSInterface->distPtLn(pConHandle, 
			pArc->endPoint()->position(),
			geEndPtsLine,
			0.0, 
			pArc->endPoint()->vcsBody(),
			pEndPtsLineBody,
			NULL, NULL,
			false, gebiasPt,
			kVCSEQ);

		// #. Constraint point on the right side 1 (distance from mid point to the endpts plane > 0)
		AcGePlane geEndPtsPlane(endptsLineSeg.midPoint(), endptsLineSeg.direction().perpVector());
		VCSMPlane endPtsPlane = GeConv::toVCSMPlane(geEndPtsPlane);
		VCSMPoint3d biasPt;
		double tolVal = 0.0;
		m_pVCSInterface->vcsSystem()->create3dDistPtPl(pConHandle,
			false, biasPt,			
			data.geom1()->vcsBody(),
			pEndPtsLineBody,
			data.geom1()->positionEx(),
			endPtsPlane,
			NULL, NULL,
			tolVal, kVCSLE);
		//data.addConsHandle(pConHandle);

		// #. Constraint point on the right side 2 (perpendicular between endPts plane normal and symLine body)
		bool res = angleConstraintEx(pConHandle,
			pEndPtsLineBody, 
			pSymLineBody, 
			geEndPtsPlane.normal(), 
			geSymLine.direction(),
			Ns::kHalfPi);
		//data.addConsHandle(pConHandle);

		// #. Constraint mid point on the symmetry line
		AcGePoint3dArray pointArray;
		pArc->circArc().getSamplePoints(3, pointArray);
		AcGePoint3d midPointOnArc = pointArray[1];

		m_pVCSInterface->distPtLn(pConHandle, 
			data.geom1()->position(),
			geSymLine,
			0.0, 
			data.geom1()->vcsBody(),
			pSymLineBody,
			NULL, NULL,
			true, midPointOnArc,
			kVCSEQ);
		data.addConsHandle(pConHandle);

		// Constraint mid point on the arc
		return circleMatePtCir(data, data.geom1()->position(), data.geom1()->vcsBody(), pArc);
	}

	return false;
}

bool SketchVCSInterface::polygonConstraint(SketchVCSConstraintData& data)
{
	// At least has 3 vertexes (2 points, 1 center)
	std::vector<SketchSolverGeometry*> geoms = data.curConsItem().geoms();
	int geomSize = (int)geoms.size();
	if (geomSize < 3)
		return false;

	// Points circle pattern base on the center axis
	if (data.checkGeomTypesAll(eSolverGeomPoint))
	{
		// Get bodies, points
		int ptsNum = geomSize - 1; // last one is the center point
		VCSRigidBody** ptBodies = new VCSRigidBody*[ptsNum];
		VCSMPoint3d* pts = new VCSMPoint3d[ptsNum];
		for (int i=0, j=0; i<ptsNum; i++, j++)
		{
			SketchSolverGeometry* pSolverGeom = geoms[i];
			ptBodies[j] = pSolverGeom->vcsBody();
			pts[j] = GeConv::toVCSMPoint3d(pSolverGeom->position());
		}
		// Get axis, use the center point at last position
		VCSRigidBody* axisBody = geoms[ptsNum]->vcsBody();
		AcGeLine3d geAxis = AcGeLine3d(geoms[ptsNum]->position(), AcGeVector3d::kZAxis);
		VCSMLine3d axis = GeConv::toVCSMLine3d(geAxis);
		double dA = (2.0 * 3.1415926) / ptsNum;

		// Radius constraint
		VCSPatternHandle* pConHandle = NULL;
		VCSStatus status = m_pVCSInterface->vcsSystem()->create3dPtsCircularPattern(
			pConHandle,
			ptBodies,
			axisBody,
			pts,
			axis,
			ptsNum,
			dA,
			true);
		data.addPatternHandle(pConHandle);
		return true;
	}

	return false;
}

bool SketchVCSInterface::symmetryConstraint(SketchVCSConstraintData& data)
{
	// At least has 3 geometries (2 elements, 1 symmetry line)
	std::vector<SketchSolverGeometry*> geoms = data.curConsItem().geoms();
	int geomSize = (int)geoms.size();
	if (geomSize != 3)
		return false;

	// Get symmetry line
	SketchSolverGeometry* pSymLine = data.curConsItem().geoms()[2];
	if (NULL == pSymLine || pSymLine->geomType() != eSolverGeomLine)
		return false;
	VCSMLine3d symLine = GeConv::toVCSMLine3d(pSymLine->line());

	// Points symmetry
	if (data.checkGeomTypes(eSolverGeomPoint))
	{
		VCSConHandle* pConHandle = NULL;
		VCSStatus status = m_pVCSInterface->vcsSystem()->create3dSymmPtPtLn(
			pConHandle,
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			data.geom1()->positionEx(),
			data.geom2()->positionEx(),
			pSymLine->vcsBody(),
			symLine);
		data.addConsHandle(pConHandle);
		return true;
	}
	// Lines symmetry
	if (data.checkGeomTypes(eSolverGeomLine))
	{
		VCSMPoint3d biasPt;
		VCSConHandle* pConHandle = NULL;
		VCSStatus status = m_pVCSInterface->vcsSystem()->create3dSymmLnLnLn(
			pConHandle,
			false,
			biasPt,
			biasPt,
			data.geom1()->vcsBody(),
			data.geom2()->vcsBody(),
			data.geom1()->lineEx(),
			data.geom2()->lineEx(),
			pSymLine->vcsBody(),
			symLine);
		data.addConsHandle(pConHandle);
		return true;
	}
	// Circle symmetry
	if (data.checkGeomTypes(eSolverGeomCirArc))
	{
		VCSConHandle* pConHandle = NULL;
		VCSStatus status = m_pVCSInterface->vcsSystem()->create3dSymmCirCirLn(
			pConHandle,
			pSymLine->vcsBody(),
			symLine,
			data.geom1()->varGeomHandle(),
			data.geom2()->varGeomHandle());
		data.addConsHandle(pConHandle);
		return true;
	}

	// Ellipse symmetry
	if (data.checkGeomTypes(eSolverGeomEllipticArc))
	{
		VCSConHandle* pConHandle = NULL;
		VCSStatus status = m_pVCSInterface->vcsSystem()->create3dSymmEllEllLn(
			pConHandle,
			pSymLine->vcsBody(),
			symLine,
			data.geom1()->varGeomHandle(),
			data.geom2()->varGeomHandle());
		data.addConsHandle(pConHandle);
		return true;
	}

	return false;
}

bool SketchVCSInterface::variableCircle(VCSVarGeomHandle*& varG, VCSRigidBody* body, const AcGeCircArc3d& geCircle)
{
	return m_pVCSInterface->variableCircle(varG, body, geCircle);
}

bool SketchVCSInterface::variableEllipse(VCSVarGeomHandle*& varG, VCSRigidBody* body, const AcGeEllipArc3d& geEllipse)
{
	VCSMEllipse3d ell = GeConv::toVCSMEllipse3d(geEllipse);
	return m_pVCSInterface->vcsSystem()->create3dVariableEllipse(varG, body, ell);
}

bool SketchVCSInterface::checkInValidConstraint(SketchVCSConstraintData& data)
{
	// Skip check if already has one
	if (data.hasInvalidConstraint())
		return true;

	if (data.consType() == eLineMidPoint)
	{
		// Point on middle of line
		if (!data.checkGeomTypes(eSolverGeomPoint, eSolverGeomLine))
			return true; // could be middle point on arc
		SketchSolverGeometry* pMidPoint = data.geom1();
		SketchSolverLine* pLine = data.geom2()->solverGeom<SketchSolverLine>(eSolverGeomLine);
		if (pLine == NULL)
			return false;

		// Same distance from midpoint to line's start/end points
		SketchSolverGeometry* pStart = pLine->solverGeomStartPt();
		SketchSolverGeometry* pEnd = pLine->solverGeomEndPt();
		double dist1 = pStart->position().distanceTo(pMidPoint->position());
		double dist2 = pEnd->position().distanceTo(pMidPoint->position());
		if (fabs(dist1 - dist2) >= 10e-6)
			data.hasInvalidConstraint(true);
		return true;
	}
	return true;
}

VCSRigidBody* SketchVCSInterface::createLineMidPointBody(SketchVCSConstraintData& data,
	SketchSolverLine* pLine)
{
	// Create mid point body
	VCSRigidBody* midPointBody = createRBody();
	AcGePoint3d midPt = pLine->lineSeg().midPoint();

	// Constraint this temp mid point on the line
	data.resetDataEX(pLine);
	VCSConHandle* pConHandle = NULL;
	m_pVCSInterface->distPtLn(pConHandle, 
		midPt, 
		pLine->line(),
		0.0, 
		midPointBody,
		pLine->vcsBody(),
		NULL, NULL,
		true, midPt,
		kVCSEQ);
	data.addConsHandle(pConHandle);

	// Create variables with equation
	double halfLineLength = pLine->length() / 2.0;
	VCSVariable* pVariable = m_pVCSInterface->createVariable(halfLineLength);
	if (!pVariable)
		return NULL;

	// Same distance from midpoint to line's start/end points
	SketchSolverGeometry* pStart = pLine->solverGeomStartPt();
	SketchSolverGeometry* pEnd = pLine->solverGeomEndPt();
	data.resetDataEX(pStart, NULL, 0.0, true);
	m_pVCSInterface->distPtPt(pConHandle, 
		data.geom1()->position(),
		midPt,
		0.0, 
		data.geom1()->vcsBody(),
		midPointBody,
		NULL, NULL, kVCSEQ, pVariable);
	data.addConsHandle(pConHandle);

	data.resetDataEX(pEnd, NULL, 0.0, true);
	m_pVCSInterface->distPtPt(pConHandle, 
		data.geom1()->position(),
		midPt,
		0.0, 
		data.geom1()->vcsBody(),
		midPointBody,
		NULL, NULL, kVCSEQ, pVariable);
	data.addConsHandle(pConHandle);

	return midPointBody;
}

VCSRigidBody* SketchVCSInterface::createConstructBodyHorizontal(SketchVCSConstraintData& data,
	SketchSolverPoint* pPoint)
{
	// Create body
	VCSRigidBody* pConsBody = createRBody();

	// Add constraint to make sure it is vertical/horizontal
	AcGeLine3d geTempLine = AcGeLine3d(pPoint->position(), m_horizontalVec);
	VCSConHandle* pConHandle = NULL;
	bool res = angleConstraintEx(pConHandle, 
		pConsBody,
		m_pHorizontalGeomBody,
		geTempLine.direction(),
		m_horizontalVec,
		0.0);

	// Point on line
	AcGePoint3d biasPt;		
	m_pVCSInterface->distPtLn(pConHandle, 
		pPoint->position(),
		geTempLine,
		0.0,
		pPoint->vcsBody(),
		pConsBody,
		NULL, NULL,
		false, biasPt,
		kVCSEQ);

	return pConsBody;
}