#include "Pch/SketchPch.h"
#include "SketchSolverCircArc.h"
#include "SketchSolverPoint.h"
#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchVCSConstraintData.h"
#include "SketchVCSDraggingData.h"
#include "../Objects/SketchGeomConstraint.h"

#include <VCSAPI.h>
#include <gemat3d.h>
#include <getol.h>
#include <Server/Component/Assembly/VCSConv.h>
#include <Server/Base/Assert/Assert.h>

// Geometries
#include <Server/Geometry/Geometry3D/Geometry3D.h>
#include <Server/Geometry/Geometry3D/CircArc3D.h>
#include <Server/Geometry/Geometry3D/Point3D.h>
#include <Server/Geometry/Geometry3D/CurvatureHandle.h>
#include <Server/Geometry/Geometry3D/TangentHandle.h>
#include <Fusion/Sketch/Server/Sketch/Objects/ConstrainedSpline3D.h>

using namespace Ns;
using namespace SKs::Constraint;
using namespace Ns::Geometry;

/// ----------------------------------------------------------------------
/// SketchSolverCircArc
/// ----------------------------------------------------------------------

SketchSolverCircArc::SketchSolverCircArc(PassiveRef<CircArc3D> geometry, SketchConstraintSolverContext* pContext)
	: SketchSolverGeometry(pContext, eSolverGeomCirArc),
	m_geometry(geometry),
	m_pVcsVarCircle(NULL),
	m_pVcsRigidBodyPtOnCircle(NULL),
	m_bHasVariableRadiusCons(false),
	m_bHasConsOnCircumference(false)
{
	updateCachedValue();

	bool bres = m_pContext->vcsInterface()->variableCircle(m_pVcsVarCircle, vcsBody(), m_geometry->arcCurve());
	NEUTRON_ASSERT(bres);

	if (m_geometry->isFixed())
		setGrounded(true);
}

SketchSolverCircArc::~SketchSolverCircArc()
{
}

bool SketchSolverCircArc::checkPreUpdateValue()
{
	if (NULL == vcsBody())
		return false;
	VCSMMatrix3d vMat = vcsBody()->transform();
	AcGeMatrix3d aMat = GeConv::toAcGeMatrix3d(vMat);

	VCSMCircle3d vcsCir = m_pVcsVarCircle->circle3d();
	double newRadius = vcsCir.radius();
	// No allow degenerated circle during solving
	if (std::fabs(newRadius) <= m_pContext->tol())
		return false;

	//if (std::fabs(newRadius) > 1000.0)
	//	return false;
	// No allow degenerated circle arc
	if (m_geometry->bounded())
	{
		AcGePoint3d startPt = startPoint()->updatedPosition();
		AcGePoint3d endPt = endPoint()->updatedPosition();
		
		if (startPt.isEqualTo(endPt, m_pContext->tolEx()))
			return false;
	}

	return true;
}

bool SketchSolverCircArc::updateValue(const AcGeMatrix3d& mat)
{
	// Need update normal, especially for 3d sketch

	// Geometry will be updated later in updateSolverGeometries (m_geometry->updateCurve();)
	VCSMCircle3d vcsCir = m_pVcsVarCircle->circle3d();
	double newRadius = vcsCir.radius();
	double oldRadius = m_geometry->arcCurve().radius();
	static double dTol = 10e-6;
	if (std::fabs(newRadius-oldRadius) <= dTol)
		return true;

	m_geometry->radius(newRadius);
	m_circArc = m_geometry->arcCurve();
	return true;
}

bool SketchSolverCircArc::onAddAdditionalConstraints(SketchVCSConstraintData& data)
{
	// Constraint center point
	const AcGePoint3d& centerPt = m_geometry->centerPoint()->point();
	data.resetData(this, centerPointSolverGeometry(), 0.0, eEndPointOnCurve);
	if (!m_pContext->vcsInterface()->coincidentConstraintPtPt(data, centerPt, centerPt))
		return false;
	
	// For Arc, constraint end points on arc
	if (m_geometry->bounded())
	{
		SketchSolverGeometry* pStartPtGeom = m_pContext->solverGeometry(m_geometry->startPoint());
		SketchSolverGeometry* pEndPtGeom = m_pContext->solverGeometry(m_geometry->endPoint());
		data.resetData(this, pStartPtGeom, 0.0, eEndPointOnCurve);
		if (!m_pContext->vcsInterface()->coincidentConstraint(data))
			return false;

		data.resetData(this, pEndPtGeom, 0.0, eEndPointOnCurve);
		if (!m_pContext->vcsInterface()->coincidentConstraint(data))
			return false;

		// Add related constraints
		if (!pStartPtGeom->addAdditionalConstraints(data))
			return false;
		if (!pEndPtGeom->addAdditionalConstraints(data))
			return false;
	}

	// For normal solving/not dragging, add constraint to make sure circle won't get degenerated
	if (!m_pContext->isDragging() && !m_pVcsVarCircle->invaraible())
	{
		// Not set invariable for all circles during solving
		if (!m_geometry->bounded() && !m_pContext->consData().hasRelatedConsWithCircleToMakeVariableInSolving(this))
			m_pVcsVarCircle->setInvariable(true);
		else
		{
			// Temporary skip if adding symmetry constraint, due to a bug in vcs (solve issue when has one circle fixed, 1477552)
			if (m_pContext->consData().newAddConstraint() != NULL &&
				m_pContext->consData().newAddConstraint()->constraintType() == SKs::eSymmetric)
				return true;
			double dMinimumRadius = m_pContext->tol();
			m_pContext->consData().resetData(this, NULL, dMinimumRadius, eInternalPointOnVariableCurve);
			m_pContext->consData().curConsItem().isMinimumValueCons(true);
			if (!m_pContext->vcsInterface()->circleRadiusConstraint(m_pContext->consData()))
				return false;
		}
	}

	return true;
}

bool SketchSolverCircArc::addHandlesForDragging(SketchVCSDraggingData& dragData)
{
	if (dragData.IsDragSingleGeom())
	{
		VCSRigidBody* pBodyOnCir = vcsBodyPtOnCircle(m_pContext->consData());
		dragData.addBodyToDrag(pBodyOnCir);

		//// Fix circle body (center position), when there has only one or zero constraint on circumference
		//if (!m_pContext->consData().hasRelatedConsWithCircleToMakeVariable(this, false))
		//	centerPointSolverGeometry()->vcsBody()->setGrounded(true);

		// Move the point on circle to the dragging position
		AcGeVector3d vecToDragStartPt = dragData.startPt() - ptOnCircle();
		AcGeMatrix3d mat;
		mat.setToTranslation(vecToDragStartPt);
		VCSMMatrix3d vMat = GeConv::toVCSMMatrix3d(mat);
		pBodyOnCir->setTransform(vMat);
	}
	else
	{
		dragData.addBodyToDrag(vcsBody());
		if (dragData.isDragGeom(centerPointSolverGeometry()))
		{
			if (NULL != m_pVcsRigidBodyPtOnCircle)
				dragData.addBodyToDrag(m_pVcsRigidBodyPtOnCircle);
		}
	}

	return true;
}

void SketchSolverCircArc::addExtraHandlesForSolving(SketchVCSConstraintData& data, SKs::CurveContinuityConstraint* c)
{
    if(!c)
    {
        return;
    }
    if(!c->containsGeometry(m_geometry))
    {
        return;
    }
    std::vector<PassiveRef<Ns::Geometry::Geometry3D> > geoms = c->geometries();
    if(geoms.size() != 2)
    {
        return;
    }

    Spline3D* sp = m_geometry == geoms[0] ? geoms[1]->query<Spline3D>() : geoms[0]->query<Spline3D>();
    if(sp == 0)
    {
        return;
    }

    bool atStart = false;
    bool outward = false;
    Spline3D::EContinuityFlag startFlag = Spline3D::eNone;
    Spline3D::EContinuityFlag endFlag = Spline3D::eNone;
    PassiveRef<Curve3D> startCurve = sp->startContinuityCurve(atStart, outward, startFlag);
    PassiveRef<Curve3D> endCurve = sp->endContinuityCurve(atStart, outward, endFlag);

    if(startCurve.isNull() && endCurve.isNull())
    {
        // should not here
        return;
    }

    bool splineIsFirst = m_geometry == geoms[1];
    bool connectedAtStart = splineIsFirst ? c->connectedAtStart1() : c->connectedAtStart2();

    // we only add constraint from this CurveContinuityConstraint
    if(startCurve == m_geometry && connectedAtStart)
    {
        std::map<int, StrongRef<TangentHandle> >::iterator it = sp->tangentUses()->tangents().find(0);
        if(it != sp->tangentUses()->tangents().end())
        {
            TangentHandle* tangent = it->second.target();
            SketchSolverGeometry* l = m_pContext->solverGeometry(tangent);
            data.resetData(l, this, 0, SKs::eTangent, false, true);
            m_pContext->vcsInterface()->tangentConstraint(data);
        }
    }
    if(endCurve == m_geometry && !connectedAtStart)
    {
        std::map<int, StrongRef<TangentHandle> >::iterator it = sp->tangentUses()->tangents().find(sp->fitPoints() - 1);
        if(it != sp->tangentUses()->tangents().end())
        {
            TangentHandle* tangent = it->second.target();
            SketchSolverGeometry* l = m_pContext->solverGeometry(tangent);
            data.resetData(l, this, 0, SKs::eTangent, false, true);
            m_pContext->vcsInterface()->tangentConstraint(data);
        }
    }
}

bool SketchSolverCircArc::addExtraHandlesForDragging(SketchVCSDraggingData& dragData)
{
	setInvariable();
	return true;
}

SketchSolverGeometry* SketchSolverCircArc::centerPointSolverGeometry()
{
	return m_pContext->solverGeometry(m_geometry->centerPoint());
}

const AcGeCircArc3d& SketchSolverCircArc::circArc() const
{
	return m_circArc;
}

VCSVarGeomHandle* SketchSolverCircArc::variableCircleHandle()
{
	return m_pVcsVarCircle;
}

VCSRigidBody* SketchSolverCircArc::vcsBodyPtOnCircle(SketchVCSConstraintData& data)
{
	// Only create this internal body when needed
	if (NULL == m_pVcsRigidBodyPtOnCircle)
	{
		m_pVcsRigidBodyPtOnCircle = m_pContext->vcsInterface()->createRBody();
		// Create a point on circle, mate it on circle
		data.resetData(NULL, NULL, 0.0, eInternalPointOnVariableCurve);
		m_pContext->vcsInterface()->circleMatePtCir(data, ptOnCircle(), m_pVcsRigidBodyPtOnCircle, this);
		data.restoreData();
	}
	return m_pVcsRigidBodyPtOnCircle;
}

bool SketchSolverCircArc::isInternalPtOnCircle(VCSRigidBody* pBody)
{
	if (NULL == m_pVcsRigidBodyPtOnCircle)
		return false;
	if (m_pVcsRigidBodyPtOnCircle == pBody)
		return true;

	return false;
}

AcGePoint3d SketchSolverCircArc::ptOnCircle()
{
	return m_geometry->arcCurve().closestPointTo(m_geometry->arcCurve().center());
}

void SketchSolverCircArc::setInvariable(bool bNotSetIfHasVariableRadiusCons)
{
	if (m_pVcsVarCircle->invaraible())
		return;
	
	// Do nothing if this circleArc itself is being dragged (refer to addHandlesForDragging)
	if (m_pContext->dragData().isDragGeom(this))
		return;

	// For arc, not set invariable if current is dragging arc's start/end point
	if (m_geometry->bounded() && m_pContext->dragData().IsDragSingleGeom())
	{
		SketchSolverGeometry* pStartPtGeom = m_pContext->solverGeometry(m_geometry->startPoint());
		SketchSolverGeometry* pEndPtGeom = m_pContext->solverGeometry(m_geometry->endPoint());
		if (m_pContext->dragData().isDragGeom(pStartPtGeom)
			|| m_pContext->dragData().isDragGeom(pEndPtGeom))
		{
			return;
		}
	}

	bool bDragCenterPoint = false;
	if (m_pContext->dragData().isDragGeom(centerPointSolverGeometry()))
		bDragCenterPoint = true;

	// Skip if there has constraint on circumference 
	if (m_pContext->consData().hasRelatedConsWithCircleToMakeVariable(this, bDragCenterPoint))
		return;

	// Not set invariable if current has special constraint (such as equal to another circle)
	// Or it would Failed to drag the circle (equal to another one) after set Invariable
	if (bNotSetIfHasVariableRadiusCons && m_bHasVariableRadiusCons)
		return;

	m_pVcsVarCircle->setInvariable(true);
}

void SketchSolverCircArc::addEntireToDrag(SketchVCSDraggingData& data)
{
	// For Arc, add start and end point geometry to drag
	if (m_geometry->bounded())
	{
		SketchSolverGeometry* pStartPtGeom = m_pContext->solverGeometry(m_geometry->startPoint());
		SketchSolverGeometry* pEndPtGeom = m_pContext->solverGeometry(m_geometry->endPoint());
		data.addGeomToDrag(pStartPtGeom);
		data.addGeomToDrag(pEndPtGeom);
	}
}

bool SketchSolverCircArc::degenerate()
{
	return m_geometry->degenerate();
}

void SketchSolverCircArc::hasVariableRadiusCons(bool val)
{
	m_bHasVariableRadiusCons = val;
}
bool SketchSolverCircArc::hasVariableRadiusCons() const
{
	return m_bHasVariableRadiusCons;
}

void SketchSolverCircArc::hasConsOnCircumference(bool val)
{
	m_bHasConsOnCircumference = val;
}

bool SketchSolverCircArc::hasConsOnCircumference()
{
	return m_bHasConsOnCircumference;
}

VCSVarGeomHandle* SketchSolverCircArc::varGeomHandle()
{
	return m_pVcsVarCircle;
}

bool SketchSolverCircArc::addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem)
{
	if (eSolverGeomPoint != pDraggedItem->geomType())
		return true;

	if (pDraggedItem == centerPointSolverGeometry())
	{
		//setInvariable(false);
		m_pContext->consData().handleSymmetryConsWhenDragging(this, true);
	}

	// For Arc, fix another end point
	//
	if (!m_geometry->bounded())
		return false;

    SketchSolverGeometry* pOtherSolverPt = NULL;
    if(m_geometry->geomType() == Geometry3D::eCurvatureHandle)
    {
        CurvatureHandle* handle = m_geometry->query<CurvatureHandle>();
        pOtherSolverPt = m_pContext->solverGeometry(handle->fitPoint());
        Spline3D* sp = (*handle->curveUses()->curves().begin())->query<Spline3D>();
        if(sp)
        {
            int index = sp->fitPointIndex(handle->fitPoint().target());
            if(index >= 0)
            {
                std::map<int, StrongRef<TangentHandle> >::iterator it = sp->tangentUses()->tangents().find(index);
                if(it != sp->tangentUses()->tangents().end())
                {
                    TangentHandle* tangent = it->second.target();
                    SketchSolverGeometry* pTangent = m_pContext->solverGeometry(tangent);
                    pTangent->setGrounded(true);
                }
            }
        }
        /*
        pTangent->addHandlesForDraggingRelatedItem(pOtherSolverPt);
        */
    }
    else
    {
        // Get the other end point
        SketchSolverGeometry* pStart = m_pContext->solverGeometry(m_geometry->startPoint());
        SketchSolverGeometry* pEnd = m_pContext->solverGeometry(m_geometry->endPoint());
        if (pDraggedItem == pStart)
            pOtherSolverPt = pEnd;
        else if (pDraggedItem == pEnd)
            pOtherSolverPt = pStart;
        else
            return true;
    }

	// Set ground
	if (m_geometry->geomType() == Geometry3D::eCurvatureHandle || m_pContext->consData().shouldFixCircleArcEndPoint(this, pOtherSolverPt))
		pOtherSolverPt->setGrounded(true);

	// Also make sure the variable added
	vcsBodyPtOnCircle(m_pContext->consData());

	return true;
}

void SketchSolverCircArc::setGrounded(bool bGrounded)
{
	SketchSolverGeometry::setGrounded(bGrounded);
	centerPointSolverGeometry()->setGrounded(bGrounded);
	// Set circle invariable if it's fixed
	m_pVcsVarCircle->setInvariable(bGrounded);
}

bool SketchSolverCircArc::isGrounded()
{
	if (vcsBody()->isGrounded())
		return true;
	if (centerPointSolverGeometry()->isGrounded())
		return true;
	return false;
}

bool SketchSolverCircArc::updateValueToLastSolved()
{
	double newRadius = m_circArc.radius();
	double oldRadius = m_geometry->arcCurve().radius();
	static double dTol = 10e-6;
	if (std::fabs(newRadius-oldRadius) <= dTol)
		return true;

	m_geometry->radius(newRadius);
	return true;
}

SketchSolverGeometry* SketchSolverCircArc::startPoint()
{
	if (m_geometry->bounded())
		return m_pContext->solverGeometry(m_geometry->startPoint());
	return NULL;
}

SketchSolverGeometry* SketchSolverCircArc::endPoint()
{
	if (m_geometry->bounded())
		return m_pContext->solverGeometry(m_geometry->endPoint());
	return NULL;
}

PassiveRef<Geometry3D> SketchSolverCircArc::geometry()
{
    return m_geometry;
}

void SketchSolverCircArc::updateCachedValue()
{
	m_position = m_geometry->centerPoint()->point();
	m_circArc = m_geometry->arcCurve();
}
