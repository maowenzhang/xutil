#include "Pch/SketchPch.h"
#include "SketchSolverEllipticArc.h"
#include "SketchSolverPoint.h"
#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchVCSConstraintData.h"
#include "SketchVCSDraggingData.h"

#include <VCSAPI.h>
#include <gemat3d.h>
#include <getol.h>
#include <geell3d.h>
#include <Server/Component/Assembly/VCSConv.h>
#include <Server/Base/Assert/Assert.h>

// Geometries
#include <Server/Geometry/Geometry3D/Point3D.h>
#include <Server/Geometry/Geometry3D/EllipticArc3D.h>

using namespace Ns;
using namespace SKs::Constraint;
using namespace Ns::Geometry;

/// ----------------------------------------------------------------------
/// SketchSolverEllipticArc
/// ----------------------------------------------------------------------

SketchSolverEllipticArc::SketchSolverEllipticArc(PassiveRef<EllipticArc3D> geometry, SketchConstraintSolverContext* pContext)
	: SketchSolverGeometry(pContext, eSolverGeomEllipticArc),
	m_geometry(geometry),
	m_pVcsVarEllipse(NULL),
	m_pInternalBodyOnEllipMajor(NULL),
	m_pInternalBodyOnEllipMinor(NULL)
{
	updateCachedValue();

	bool bres = m_pContext->vcsInterface()->variableEllipse(m_pVcsVarEllipse, vcsBody(), m_ellipticArc);
	NEUTRON_ASSERT(bres);
	if (m_geometry->isFixed())
		setGrounded(true);
}

SketchSolverEllipticArc::~SketchSolverEllipticArc()
{
}

bool SketchSolverEllipticArc::checkPreUpdateValue()
{
	if (NULL == vcsBody())
		return false;

	// No allow degenerated during solving
	VCSMEllipse3d vcsEllipse = m_pVcsVarEllipse->ellipse3d();
	double newMajorRadius = vcsEllipse.majorRadius();
	if (std::fabs(newMajorRadius) <= m_pContext->tol())
		return false;
	double newMinorRadius = vcsEllipse.minorRadius();
	if (std::fabs(newMinorRadius) <= m_pContext->tol())
		return false;

	// No allow degenerated
	if (m_geometry->bounded())
	{
		AcGePoint3d startPt = startPoint()->updatedPosition();
		AcGePoint3d endPt = endPoint()->updatedPosition();
		
		if (startPt.isEqualTo(endPt, m_pContext->tolEx()))
			return false;
	}

	return true;
}

bool SketchSolverEllipticArc::updateValue(const AcGeMatrix3d& mat)
{
	// Need update normal, especially for 3d sketch

	// Geometry will be updated later in updateSolverGeometries (m_geometry->updateCurve();)
	VCSMEllipse3d vcsEllipse = m_pVcsVarEllipse->ellipse3d();
	AcGeEllipArc3d geEllipse = GeConv::toAcGeEllipArc3d(vcsEllipse);

	double newMajorRadius = vcsEllipse.majorRadius();
	double newMinorRadius = vcsEllipse.minorRadius();
	m_geometry->majorRadius(newMajorRadius);
	m_geometry->minorRadius(newMinorRadius);

	m_geometry->setAxis(geEllipse.majorAxis(), geEllipse.minorAxis());
	m_ellipticArc = m_geometry->arcCurve();
	return true;
}

bool SketchSolverEllipticArc::onAddAdditionalConstraints(SketchVCSConstraintData& data)
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
	if (!m_pContext->isDragging())
	{
		if (!m_pContext->consData().hasRelatedConsWithCircleToMakeVariableInSolving(this))
			m_pVcsVarEllipse->setInvariable(true);
		else
		{
			double dMinimumRadius = m_pContext->tol();
			m_pContext->consData().resetData(this, NULL, dMinimumRadius, eInternalPointOnVariableCurve);
			m_pContext->consData().curConsItem().isMinimumValueCons(true);
			if (!m_pContext->vcsInterface()->ellipseRadiusConstraint(m_pContext->consData()))
				return false;
		}
	}

	return true;
}

bool SketchSolverEllipticArc::isDragChangingMajorAxis(SketchVCSDraggingData& dragData, double& posParam)
{
	AcGePoint3d ptOnEllp = m_ellipticArc.closestPointTo(dragData.startPt());
	posParam = m_ellipticArc.paramOf(ptOnEllp);
	AcGeLine3d major(m_ellipticArc.center(), m_ellipticArc.majorAxis());
	AcGeLine3d minor(m_ellipticArc.center(), m_ellipticArc.minorAxis());
	
	// Get distance from startPt to end point of major axis
	AcGePoint3d p1, p2;
	int intNum = 0;
	if (!m_ellipticArc.intersectWith(major, intNum, p1, p2) || intNum != 2)
	{
		NEUTRON_ASSERT(false);
		return false;
	}
	double distToMajorAxis = std::min(p1.distanceTo(ptOnEllp), p2.distanceTo(ptOnEllp));

	// Get distance from startPt to end point of minor axis
	if (!m_ellipticArc.intersectWith(minor, intNum, p1, p2) || intNum != 2)
	{
		NEUTRON_ASSERT(false);
		return false;
	}
	double distToMinorAxis = std::min(p1.distanceTo(ptOnEllp), p2.distanceTo(ptOnEllp));

	if (distToMajorAxis < distToMinorAxis)
		return true;
	return false;
}

bool SketchSolverEllipticArc::addHandlesForDragging(SketchVCSDraggingData& dragData)
{
	if (dragData.IsDragSingleGeom())
	{
		double posParam = 0.0;
		if (isDragChangingMajorAxis(dragData, posParam))
		{
			VCSRigidBody* pBodyOnCir = internalBodyOnEllipMajor(m_pContext->consData(), posParam);
			dragData.addBodyToDrag(pBodyOnCir);
			m_pVcsVarEllipse->setMinorRadiusInvariable(true);
		}
		else
		{
			VCSRigidBody* pBodyOnCir = internalBodyOnEllipMinor(m_pContext->consData(), posParam);
			dragData.addBodyToDrag(pBodyOnCir);
			m_pVcsVarEllipse->setMajorRadiusInvariable(true);
		}	

		// Fix circle body (center position) in some cases
		if (!m_pContext->consData().hasRelatedConsWithCircleToMakeVariable(this, false))
			centerPointSolverGeometry()->vcsBody()->setGrounded(true);
	}
	else
	{
		dragData.addBodyToDrag(vcsBody());
		if (dragData.isDragGeom(centerPointSolverGeometry()))
		{
			if (NULL != m_pInternalBodyOnEllipMajor)
				dragData.addBodyToDrag(m_pInternalBodyOnEllipMajor);
			if (NULL != m_pInternalBodyOnEllipMinor)
				dragData.addBodyToDrag(m_pInternalBodyOnEllipMinor);
		}
	}

	return true;
}

bool SketchSolverEllipticArc::addExtraHandlesForDragging(SketchVCSDraggingData& dragData)
{
	setInvariable();
	return true;
}

VCSVarGeomHandle* SketchSolverEllipticArc::variableEllipseHandle()
{
	return m_pVcsVarEllipse;
}

VCSRigidBody* SketchSolverEllipticArc::internalBodyOnEllip(SketchVCSConstraintData& data, AcGePoint3d& ptOnEllipse)
{
	VCSRigidBody* pBody = m_pContext->vcsInterface()->createRBody();

	// Create a point on ellipse, mate it on ellipse
	data.resetData(NULL, NULL, 0.0, eInternalPointOnVariableCurve);
	m_pContext->vcsInterface()->matePtEllipse(data, ptOnEllipse, pBody, this, true, ptOnEllipse, true);
	data.restoreData();
	return pBody;
}

VCSRigidBody* SketchSolverEllipticArc::internalBodyOnEllipMajor(SketchVCSConstraintData& data, double posParam)
{
	// Only create this internal body when needed
	if (NULL == m_pInternalBodyOnEllipMajor)
	{
		AcGePoint3d ptOnEllip;
		if (posParam < 0.0)
			ptOnEllip = getEllipMajorPt();
		else
			ptOnEllip = m_geometry->arcCurve().evalPoint(posParam);
		m_pInternalBodyOnEllipMajor = internalBodyOnEllip(data, ptOnEllip);
	}
	return m_pInternalBodyOnEllipMajor;
}

VCSRigidBody* SketchSolverEllipticArc::internalBodyOnEllipMinor(SketchVCSConstraintData& data, double posParam)
{
	// Only create this internal body when needed
	if (NULL == m_pInternalBodyOnEllipMinor)
	{
		AcGePoint3d ptOnEllip;
		if (posParam < 0.0)
			ptOnEllip = getEllipMinorPt();
		else
			ptOnEllip = m_geometry->arcCurve().evalPoint(posParam);
		m_pInternalBodyOnEllipMinor = internalBodyOnEllip(data, ptOnEllip);
	}
	return m_pInternalBodyOnEllipMinor;
}

void SketchSolverEllipticArc::setInvariable()
{
	if (m_pVcsVarEllipse->invaraible())
		return;
	
	// Do nothing if itself is being dragged (refer to addHandlesForDragging)
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

	m_pVcsVarEllipse->setInvariable(true);
}

bool SketchSolverEllipticArc::addExtraGeomsForDragging(SketchVCSDraggingData& dragData)
{
	// Do nothing
	return true;
}

const AcGeEllipArc3d& SketchSolverEllipticArc::ellipArc() const
{
	return m_geometry->arcCurve();
}

SketchSolverGeometry* SketchSolverEllipticArc::centerPointSolverGeometry()
{
	return m_pContext->solverGeometry(m_geometry->centerPoint());
}

bool SketchSolverEllipticArc::degenerate()
{
	return m_geometry->degenerate();
}

VCSVarGeomHandle* SketchSolverEllipticArc::varGeomHandle()
{
	return m_pVcsVarEllipse;
}

void SketchSolverEllipticArc::addEntireToDrag(SketchVCSDraggingData& data)
{
	// For Arc, add start and end point geometry to drag
	if (m_geometry->bounded())
	{
		SketchSolverGeometry* pStartPtGeom = m_pContext->solverGeometry(m_geometry->startPoint());
		SketchSolverGeometry* pEndPtGeom = m_pContext->solverGeometry(m_geometry->endPoint());
		data.addGeomToDrag(pStartPtGeom);
		data.addGeomToDrag(pEndPtGeom);
	}
	else
	{
		// Add ellipse to drag together if it has symmetry constraint on it
		// As an issue related with dragging VCS ellipse with symmetry, 
		// when drag center point, ellipse couldn't be moved at specified position (FUS-6268)
		if (m_pContext->consData().hasSymmetryConstraintOnGeom(this))
		{
			data.addGeomToDrag(this);
		}
	}
}

bool SketchSolverEllipticArc::addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem)
{
	if (eSolverGeomPoint != pDraggedItem->geomType())
		return true;

	if (pDraggedItem == centerPointSolverGeometry())
	{
		m_pContext->consData().handleSymmetryConsWhenDragging(this, true);
	}

	// For Arc, fix another end point
	//
	if (!m_geometry->bounded())
		return false;

    SketchSolverGeometry* pOtherSolverPt = NULL;    
    // Get the other end point
    SketchSolverGeometry* pStart = m_pContext->solverGeometry(m_geometry->startPoint());
    SketchSolverGeometry* pEnd = m_pContext->solverGeometry(m_geometry->endPoint());
    if (pDraggedItem == pStart)
        pOtherSolverPt = pEnd;
    else if (pDraggedItem == pEnd)
        pOtherSolverPt = pStart;
    else
        return true;

	// Set ground
	if (m_pContext->consData().shouldFixCircleArcEndPoint(this, pOtherSolverPt))
		pOtherSolverPt->setGrounded(true);

	// Also make sure the variable added
	internalBodyOnEllipMajor(m_pContext->consData());
	internalBodyOnEllipMinor(m_pContext->consData());

	return true;
}

void SketchSolverEllipticArc::setGrounded(bool bGrounded)
{
	SketchSolverGeometry::setGrounded(bGrounded);
	centerPointSolverGeometry()->setGrounded(bGrounded);
	// Set circle invariable if it's fixed
	m_pVcsVarEllipse->setInvariable(bGrounded);
}

bool SketchSolverEllipticArc::updateValueToLastSolved()
{
	m_geometry->majorRadius(m_ellipticArc.majorRadius());
	m_geometry->minorRadius(m_ellipticArc.minorRadius());
	m_geometry->setAxis(m_ellipticArc.majorAxis(), m_ellipticArc.minorAxis());
	return true;
}

SketchSolverGeometry* SketchSolverEllipticArc::startPoint()
{
	if (m_geometry->bounded())
		return m_pContext->solverGeometry(m_geometry->startPoint());
	return NULL;
}

SketchSolverGeometry* SketchSolverEllipticArc::endPoint()
{
	if (m_geometry->bounded())
		return m_pContext->solverGeometry(m_geometry->endPoint());
	return NULL;
}

AcGePoint3d SketchSolverEllipticArc::getEllipMajorPt()
{
	AcGeVector3d majorAxis = m_ellipticArc.majorAxis();
	AcGePoint3d majorPt = m_ellipticArc.center() + majorAxis * m_ellipticArc.majorRadius();
	return majorPt;
}

AcGePoint3d SketchSolverEllipticArc::getEllipMinorPt()
{
	AcGeVector3d minorAxis = m_ellipticArc.minorAxis();
	AcGePoint3d minorPt = m_ellipticArc.center() + minorAxis * m_ellipticArc.minorRadius();
	return minorPt;
}

PassiveRef<Geometry3D> SketchSolverEllipticArc::geometry()
{
    return m_geometry;
}

void SketchSolverEllipticArc::updateCachedValue()
{
	m_ellipticArc = m_geometry->arcCurve();
	m_position = m_geometry->centerPoint()->point();
}
