#include "Pch/SketchPch.h"
#include "SketchSolverLine.h"
#include "SketchSolverPoint.h"
#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchVCSConstraintData.h"
#include "SketchVCSDraggingData.h"
#include "SketchSolverReactor.h"
#include "../Objects/SketchGeomConstraint.h"

#include <VCSAPI.h>
#include <gemat3d.h>
#include <getol.h>
#include <Server/Component/Assembly/VCSConv.h>
#include <Server/Base/Assert/Assert.h>

// Geometries
#include <Server/Geometry/Geometry3D/Geometry3D.h>
#include <Server/Geometry/Geometry3D/Line3D.h>
#include <Server/Geometry/Geometry3D/TangentHandle.h>
#include <Server/Geometry/Geometry3D/Point3D.h>
#include <Server/Geometry/Geometry3D/Spline3D.h>
#include <Server/Geometry/Geometry3D/CurvatureHandle.h>

using namespace Ns;
using namespace SKs::Constraint;
using namespace Ns::Geometry;

/// ----------------------------------------------------------------------
/// SketchSolverLine
/// ----------------------------------------------------------------------

SketchSolverLine::SketchSolverLine(PassiveRef<Line3D> geometry, SketchConstraintSolverContext* pContext)
	: SketchSolverGeometry(pContext, eSolverGeomLine),
	m_geometry(geometry),
	m_tempLine(geometry->start(), geometry->end()),
	m_pReactor(NULL),
	m_pMidPointBody(NULL),
	m_bUseCombinedBody(false)
{
	if (m_geometry->isFixed())
		vcsBody()->setGrounded(true);
	m_position = geometry->line().midPoint();
}

SketchSolverLine::~SketchSolverLine()
{
	// VCS Reactor is managed by VCS system
	m_pReactor = NULL;
	m_pMidPointBody = NULL;
}

bool SketchSolverLine::updateValue(const AcGeMatrix3d& mat)
{
	// End points should have been updated by SketchSolverPoint
	// Geometry will be updated later in updateSolverGeometries (m_geometry->updateCurve();)
	return true;
}

bool SketchSolverLine::checkPreUpdateValue()
{
	VCSMMatrix3d vMatStartPt = solverGeomStartPt()->vcsBody()->transform();
	AcGeMatrix3d aMatStartPt = GeConv::toAcGeMatrix3d(vMatStartPt);

	VCSMMatrix3d vMatEndPt = solverGeomEndPt()->vcsBody()->transform();
	AcGeMatrix3d aMatEndPt = GeConv::toAcGeMatrix3d(vMatEndPt);

	if (aMatStartPt.isEqualTo(AcGeMatrix3d::kIdentity)
		&& aMatEndPt.isEqualTo(AcGeMatrix3d::kIdentity))
		return true;

	AcGePoint3d startPt = solverGeomStartPt()->position();
	if (!aMatStartPt.isEqualTo(AcGeMatrix3d::kIdentity))
		startPt.transformBy(aMatStartPt);
	AcGePoint3d endPt = solverGeomEndPt()->position();
	if (!aMatEndPt.isEqualTo(AcGeMatrix3d::kIdentity))
		endPt.transformBy(aMatEndPt);

	// Not allow degenerated line during solving 
	// will consider support degenerate geometries later which require some improvements from VCS, zhanglo
	AcGeTol tol;
	tol.setEqualPoint(1.0e-5);
	if (startPt.isEqualTo(endPt, tol))
		return false;

	return true;
}

bool SketchSolverLine::onAddAdditionalConstraints(SketchVCSConstraintData& data)
{
	// Add two coincident constraints (two end points on sketch line)
	SketchSolverGeometry* pGeom1 = m_pContext->solverGeometry(m_geometry->startPoint());
	SketchSolverGeometry* pGeom2 = m_pContext->solverGeometry(m_geometry->endPoint());

	if (!useCombinedBody())
	{
		data.resetData(pGeom1, this, 0.0, eEndPointOnCurve);
		if (!m_pContext->vcsInterface()->coincidentConstraint(data))
			return false;

		data.resetData(pGeom2, this, 0.0, eEndPointOnCurve);
		if (!m_pContext->vcsInterface()->coincidentConstraint(data))
			return false;
	}

	// Handle associated geometries if hasn't
	if (!pGeom1->addAdditionalConstraints(data))
		return false;
	if (!pGeom2->addAdditionalConstraints(data))
		return false;

	// Add kVCSGE constraint to each line's end points, to avoid the geometry degeneration in solve
	// Also related with constraint redundancy check
	if (!m_pContext->isDragging())
	{
		if (!useCombinedBody())
		{
			SketchSolverGeometry* pGeom1 = m_pContext->solverGeometry(m_geometry->startPoint());
			SketchSolverGeometry* pGeom2 = m_pContext->solverGeometry(m_geometry->endPoint());
			double dMinimumLength = m_pContext->tol();
			m_pContext->consData().resetData(pGeom1, pGeom2, dMinimumLength, eLinearDim);
			m_pContext->consData().curConsItem().isMinimumValueCons(true);
			if (!m_pContext->vcsInterface()->distanceConstraint(m_pContext->consData()))
				return false;
		}
	}

	return true;
}

void SketchSolverLine::addExtraHandlesForSolving(SketchVCSConstraintData& data, SKs::CurveContinuityConstraint* c)
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
    // check if this constraint is on start point or end point?
    bool connectedAtStart = splineIsFirst ? c->connectedAtStart1() : c->connectedAtStart2();

    // we only add constraint from this CurveContinuityConstraint
    if(m_geometry->geomType() == Geometry3D::eTangentHandle)
    {
        if(startCurve.isNotNull() && connectedAtStart)
        {
            std::map<int, StrongRef<TangentHandle> >::iterator it = sp->tangentUses()->tangents().find(0);
            if(it != sp->tangentUses()->tangents().end())
            {
                SketchSolverGeometry* l = m_pContext->solverGeometry(startCurve);
                data.resetData(l, this, 0, SKs::eParallel, false, true);
                m_pContext->vcsInterface()->parallelConstraint(data);
            }
        }
        if(endCurve.isNotNull() && !connectedAtStart)
        {
            std::map<int, StrongRef<TangentHandle> >::iterator it = sp->tangentUses()->tangents().find(sp->fitPoints() - 1);
            if(it != sp->tangentUses()->tangents().end())
            {
                SketchSolverGeometry* l = m_pContext->solverGeometry(endCurve);
                data.resetData(l, this, 0, SKs::eParallel, false, true);
                m_pContext->vcsInterface()->parallelConstraint(data);
            }
        }

        return;
    }
    if(startCurve == m_geometry && connectedAtStart)
    {
        std::map<int, StrongRef<TangentHandle> >::iterator it = sp->tangentUses()->tangents().find(0);
        if(it != sp->tangentUses()->tangents().end())
        {
            TangentHandle* tangent = it->second.target();
            SketchSolverGeometry* l = m_pContext->solverGeometry(tangent);
            data.resetData(l, this, 0, SKs::eParallel, false, true);
            m_pContext->vcsInterface()->parallelConstraint(data);
        }
    }
    // we only add constraint from this CurveContinuityConstraint
    if(endCurve == m_geometry && !connectedAtStart)
    {
        std::map<int, StrongRef<TangentHandle> >::iterator it = sp->tangentUses()->tangents().find(sp->fitPoints() - 1);
        if(it != sp->tangentUses()->tangents().end())
        {
            TangentHandle* tangent = it->second.target();
            SketchSolverGeometry* l = m_pContext->solverGeometry(tangent);
            data.resetData(l, this, 0, SKs::eParallel, false, true);
            m_pContext->vcsInterface()->parallelConstraint(data);
        }
    }
}

bool SketchSolverLine::fixLength()
{
    SketchSolverGeometry* pStartPt = solverGeomStartPt();
    SketchSolverGeometry* pEndPt = solverGeomEndPt();
    double dLength = pStartPt->position().distanceTo(pEndPt->position());
    m_pContext->consData().resetData(pStartPt, pEndPt, dLength, eLinearDim);

    if (!m_pContext->vcsInterface()->distanceConstraint(m_pContext->consData()))
        return false;
    return true;
}

bool SketchSolverLine::addHandlesForDragging(SketchVCSDraggingData& dragData)
{
    dragData.addBodyToDrag(vcsBody());
    return true;
}

bool SketchSolverLine::addExtraGeomsForDragging(SketchVCSDraggingData& dragData)
{
    // Add start/end point to drag together
    // As long as not other geometry connected with the point, add to drag together
    // If the point is fixed, it also works. 
	// If the point is constrained, then this line Failed to rotate which should be acceptable,
	// (rotate is better could be done via temp dragged geom)
	bool bAddStartPt = false;
	bool bAddEndPt = false;	
	if (m_geometry->startPoint()->curveUses()->geometriesCount() == 1)
	{
		bAddStartPt = true;
		dragData.addGeomToDrag(solverGeomStartPt());
	}
	if (m_geometry->endPoint()->curveUses()->geometriesCount() == 1)
	{
		bAddEndPt = true;
		dragData.addGeomToDrag(solverGeomEndPt());
	}

	// Add fit point to drag together for tangent handle
	if (bAddEndPt && bAddStartPt)
	{
		if (m_geometry->geomType() == Geometry3D::eTangentHandle)
		{
			TangentHandle* handle = m_geometry->query<TangentHandle>();
			if(handle->fitPoint().isNotNull() && handle->fitPoint()->curveUses()->geometriesCount() == 1)
			{
				dragData.addGeomToDrag(m_pContext->solverGeometry(handle->fitPoint()));
                    /*
                Spline3D* sp = (*handle->curveUses()->curves().begin())->query<Spline3D>();
                int index = sp->fitPointIndex(handle->fitPoint().target());
                std::map<int, StrongRef<CurvatureHandle> >::iterator it = sp->curvatureUses()->curvatures().find(index);
                if(it != sp->curvatureUses()->curvatures().end())
                {
                    SketchSolverGeometry* pHandle = m_pContext->solverGeometry(it->second->startPoint());
                    dragData.addGeomToDrag(pHandle);

                    pHandle = m_pContext->solverGeometry(it->second->endPoint());
                    dragData.addGeomToDrag(pHandle);

                    pHandle = m_pContext->solverGeometry(it->second->centerPoint());
                    dragData.addGeomToDrag(pHandle);

                    pHandle = m_pContext->solverGeometry(it->second);
                    dragData.addGeomToDrag(pHandle);

//                    it->second->centerPoint()->isVisible(true);
                }
            */
            }
        }
    }

	return true;
}

bool SketchSolverLine::addExtraHandlesForDragging(SketchVCSDraggingData& dragData)
{
	return true;
}

const AcGeLine3d& SketchSolverLine::line()
{
	m_tempLine = AcGeLine3d(m_geometry->start(), m_geometry->end());
	return m_tempLine;
}

AcGeVector3d SketchSolverLine::lineDirection()
{
	if (m_geometry->degenerate())
		return AcGeVector3d::kXAxis;

	return line().direction();
}

const AcGeLineSeg3d& SketchSolverLine::lineSeg()
{
	return m_geometry->line();
}

double SketchSolverLine::length() const
{
	return m_geometry->line().length();
}

SketchSolverGeometry* SketchSolverLine::solverGeomStartPt()
{
	return m_pContext->solverGeometry(m_geometry->startPoint());
}

SketchSolverGeometry* SketchSolverLine::solverGeomEndPt()
{
	return m_pContext->solverGeometry(m_geometry->endPoint());
}

bool SketchSolverLine::degenerate()
{
	return m_geometry->degenerate();
}

int SketchSolverLine::onReactorNotify(int iVCSMessage)
{
	// On kSetFormed (line and its two end points),
	// we need to set its COR to make the line rotates based on its center point
	VCSMessage eVcsMessage = (VCSMessage)iVCSMessage;
	if (eVcsMessage != kSetFormed)
		return 1;

	// Get middle point
	AcGeLineSeg3d curline = AcGeLineSeg3d(m_geometry->start(), m_geometry->end());
	AcGePoint3d midPt = curline.midPoint();
	VCSMPoint3d vcsMidPt = GeConv::toVCSMPoint3d(midPt);

	// Get supper body and set its COR
	VCSRigidBody* pSupperBody = vcsBody()->parentBody();
	if (NULL != pSupperBody &&  pSupperBody->isSuperBody())
	{
		if (NULL == pSupperBody->getBodyState().bodyState())
			return 1;
		pSupperBody->setCOR(vcsMidPt);
	}

	return 1;
}

PassiveRef<Geometry3D> SketchSolverLine::geometry()
{
    return m_geometry;
}

void SketchSolverLine::fixAnotherEndPointWhenDrag(SketchSolverGeometry* pSolverPt)
{
	SketchSolverGeometry* ptStartPt = solverGeomStartPt();
	SketchSolverGeometry* ptEndPt = solverGeomEndPt();
	bool bIsDragEndPoint = false;
	if (pSolverPt == ptStartPt || pSolverPt == ptEndPt)
		bIsDragEndPoint = true;

	if (bIsDragEndPoint)
		m_pContext->consData().handleSymmetryConsWhenDragging(this, true);

	// Skip if the line has related constraints (such as horizontal, parallel, etc)
	if (m_geometry->geomType() != Geometry3D::eTangentHandle && m_pContext->consData().hasRelatedConsWithLinePreventFixEndPoint(this))
    {
        if (bIsDragEndPoint)
        {
            m_pContext->consData().handleConstrainedHandleWhenDragging(this, m_pContext);
        }
        return;
    }

	if (bIsDragEndPoint)
	{
		// Fix one end point or fit point if it's tangent handle
		SketchSolverGeometry* ptToFix = ptStartPt;
		if (pSolverPt == ptStartPt)
			ptToFix = ptEndPt;

        if (m_geometry->geomType() == Geometry3D::eTangentHandle)
        {
            TangentHandle* handle = m_geometry->query<TangentHandle>();
            ptToFix = m_pContext->solverGeometry(handle->fitPoint());
            m_pContext->consData().handleConstrainedHandleWhenDragging(this, m_pContext);
            // the fit point will be always fixed
            // see FUS-5893, FUS-6429
			ptToFix->setGrounded(true);
            return;
        }
		
		if (m_pContext->consData().shouldFixLineEndPoint(this, pSolverPt, ptToFix))
        {
			ptToFix->setGrounded(true);
            m_pContext->consData().handleConstrainedHandleWhenDragging(this, m_pContext);
        }
		return;
	}
	
	// We don't ground point if user is dragging a fit point
	if (m_geometry->geomType() == Geometry3D::eTangentHandle)
		return;

	// The input is a point on the line (including mid point)
	SketchSolverGeometry* ptToFix = ptStartPt;
	SketchSolverGeometry* ptRemainOne = ptEndPt;
	if (ptStartPt->position().y < ptEndPt->position().y)
	{
		ptToFix = ptEndPt;
		ptRemainOne = ptStartPt;
	}
	if (m_pContext->consData().hasRelatedConsBetweenGeometry(pSolverPt, ptToFix))
		return;
	// Only fix it if another point is not grounded
	if (!ptRemainOne->vcsBody()->isGrounded())
		ptToFix->setGrounded(true);
}

void SketchSolverLine::addReactor()
{
	if (NULL != m_pReactor)
		return;
	m_pReactor = new SketchSolverReactor(this);
	m_pContext->vcsInterface()->vcsSystem()->attachBodyToReactor(m_pReactor, vcsBody());
}

bool SketchSolverLine::addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem)
{
	if (useCombinedBody())
		return true;

	if (eSolverGeomPoint == pDraggedItem->geomType())
		fixAnotherEndPointWhenDrag(pDraggedItem);

	return true;
}

const AcGePoint3d& SketchSolverLine::position()
{
	m_position = m_geometry->line().midPoint();
	return m_position;
}

VCSRigidBody* SketchSolverLine::getMidPointBody()
{
	if (NULL == m_pMidPointBody)
	{
		m_pMidPointBody = m_pContext->vcsInterface()->createLineMidPointBody(m_pContext->consData(), this);
	}
	return m_pMidPointBody;
}

bool SketchSolverLine::useCombinedBody() const
{
	SketchSolverGeometry* pGeom1 = m_pContext->solverGeometry(m_geometry->startPoint());
	SketchSolverGeometry* pGeom2 = m_pContext->solverGeometry(m_geometry->endPoint());
	if (pGeom1->useCombinedBody() && pGeom2->useCombinedBody())
		return true;
	return false;
}

void SketchSolverLine::useCombinedBody(bool bVal)
{
	// Only support set as true
	NEUTRON_ASSERT(bVal);

	SketchSolverGeometry* pGeom1 = m_pContext->solverGeometry(m_geometry->startPoint());
	SketchSolverGeometry* pGeom2 = m_pContext->solverGeometry(m_geometry->endPoint());
	// Not use combine body if any end point is already used
	if (pGeom1->useCombinedBody() || pGeom2->useCombinedBody())
		return;
	// Not use combine body if any end point is fixed (e.g. drag end point to rotate line)
	if (pGeom1->isGrounded() || pGeom2->isGrounded())
		return;

	pGeom1->useCombinedBody(this);
	pGeom2->useCombinedBody(this);

	m_bUseCombinedBody = bVal;
}