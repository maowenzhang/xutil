#include "Pch/SketchPch.h"
#include "SketchSolverPoint.h"
#include "SketchSolverLine.h"
#include "SketchSolverCircArc.h"
#include "SketchSolverEllipticArc.h"
#include "SketchSolverSpline.h"
#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchVCSConstraintData.h"
#include "SketchVCSDraggingData.h"
#include "SketchSolverReactor.h"

#include <VCSAPI.h>
#include <gemat3d.h>
#include <getol.h>
#include <Server/Component/Assembly/VCSConv.h>
#include <Server/Base/Assert/Assert.h>

// Geometries
#include <Server/Geometry/Geometry3D/Geometry3D.h>
#include <Server/Geometry/Geometry3D/Line3D.h>
#include <Server/Geometry/Geometry3D/Point3D.h>
#include <Server/Geometry/Geometry3D/Spline3D.h>
#include <Server/Geometry/Geometry3D/TangentHandle.h>
#include <Server/Geometry/Geometry3D/CurvatureHandle.h>

using namespace Ns;
using namespace SKs::Constraint;
using namespace Ns::Geometry;

/// ----------------------------------------------------------------------
/// SketchSolverPoint
/// ----------------------------------------------------------------------

SketchSolverPoint::SketchSolverPoint(PassiveRef<Point3D> geometry, SketchConstraintSolverContext* pContext)
	: SketchSolverGeometry(pContext, eSolverGeomPoint),
	m_geometry(geometry),
	m_pConsBodyHorizontal(NULL),
	m_pConsBodyVertical(NULL),
	m_pCombinedLine(NULL)
{
	if (m_geometry->isFixed())
		vcsBody()->setGrounded(true);
	updateCachedValue();
}

SketchSolverPoint::~SketchSolverPoint()
{
}

bool SketchSolverPoint::updateValue(const AcGeMatrix3d& mat)
{
	m_pContext->addUpdatedPoint(m_geometry);

	AcGePoint3d pt = m_geometry->point();
	pt.transformBy(mat);
	m_geometry->point(pt);
	m_position = pt;
	return true;
}

bool SketchSolverPoint::onAddAdditionalConstraints(SketchVCSConstraintData& data)
{
	StrongRef<CurveUses> curveUses = m_geometry->curveUses();
	std::vector< PassiveRef<Curve3D> >& curveList = curveUses->curves();
	BOOST_FOREACH(PassiveRef<Curve3D> curve, curveList)
	{
		SketchSolverGeometry* pGeom = m_pContext->solverGeometry(curve);
		pGeom->addAdditionalConstraints(data);
	}
	return true;
}

bool SketchSolverPoint::addHandlesForDragging(SketchVCSDraggingData& dragData)
{
	dragData.addBodyToDrag(vcsBody());	

	if (dragData.IsDragSingleGeom())
	{
		StrongRef<CurveUses> curveUses = m_geometry->curveUses();
		std::vector< PassiveRef<Curve3D> >& curveList = curveUses->curves();
		BOOST_FOREACH(PassiveRef<Curve3D> curve, curveList)
		{
			SketchSolverGeometry* pGeom = m_pContext->solverGeometry(curve);
			pGeom->addHandlesForDraggingRelatedItem(this);
		}

		// #. For line's mid point or point on line
		// Check whether this is the point on line
		std::vector<SketchSolverLine*> relatedLines;
		if (m_pContext->consData().getRelatedLinesFromPoint(this, relatedLines))
		{
			BOOST_FOREACH(SketchSolverLine* pLine, relatedLines)
			{
				pLine->addHandlesForDraggingRelatedItem(this);
			}
		}
	}

	return true;
}

bool SketchSolverPoint::addExtraGeomsForDragging(SketchVCSDraggingData& dragData)
{
	if (!dragData.IsDragSingleGeom())
		return true;

	// Add referenced circleArc/ellipticArc to drag together if this is the center point
	// To support case: drag whole arc (including start/end points) when drag it's center point
	StrongRef<CurveUses> curveUses = m_geometry->curveUses();
	std::vector< PassiveRef<Curve3D> >& curveList = curveUses->curves();
	BOOST_FOREACH(PassiveRef<Curve3D> curve, curveList)
	{
		SketchSolverGeometry* pGeom = m_pContext->solverGeometry(curve);
		if (pGeom->geomType() == eSolverGeomCirArc)
		{
			SketchSolverCircArc* pCircle = pGeom->solverGeom<SketchSolverCircArc>(eSolverGeomCirArc);
			if (this == pCircle->centerPointSolverGeometry())
			{
				pCircle->addEntireToDrag(dragData);
			}
		}
		else if (pGeom->geomType() == eSolverGeomEllipticArc)
		{
			SketchSolverEllipticArc* pEllipse = pGeom->solverGeom<SketchSolverEllipticArc>(eSolverGeomEllipticArc);
			if (this == pEllipse->centerPointSolverGeometry())
			{
				pEllipse->addEntireToDrag(dragData);
			}
		}
		else if (pGeom->geomType() == eSolverGeomSpline)
        {
			SketchSolverSpline* pSpline = pGeom->solverGeom<SketchSolverSpline>(eSolverGeomSpline);
            Spline3D* sp = pSpline->geometry()->query<Spline3D>();
            int id = sp->fitPointIndex(m_geometry.target());
            if(id >= 0)
            {
                if(sp->hasTangent(id))
                {
                    std::map<int, StrongRef<TangentHandle> >::iterator it = sp->tangentUses()->tangents().find(id);
                    if(it != sp->tangentUses()->tangents().end())
                    {
                        SketchSolverGeometry* pHandle = m_pContext->solverGeometry(it->second->startPoint());
                        dragData.addGeomToDrag(pHandle);
                        pHandle = m_pContext->solverGeometry(it->second->endPoint());
                        dragData.addGeomToDrag(pHandle);

                        pHandle = m_pContext->solverGeometry(it->second);
                        dragData.addGeomToDrag(pHandle);
                    }
                }
                if(sp->hasCurvature(id))
                {
                    std::map<int, StrongRef<CurvatureHandle> >::iterator it = sp->curvatureUses()->curvatures().find(id);
                    if(it != sp->curvatureUses()->curvatures().end())
                    {
                        SketchSolverGeometry* pHandle = m_pContext->solverGeometry(it->second->startPoint());
                        dragData.addGeomToDrag(pHandle);
                        pHandle = m_pContext->solverGeometry(it->second->endPoint());
                        dragData.addGeomToDrag(pHandle);

                        pHandle = m_pContext->solverGeometry(it->second);
                        dragData.addGeomToDrag(pHandle);
                    }
                }
            }
        }
	}

	// For polygon constraint
	m_pContext->consData().handlePolygonConsWhenDragging(this, dragData);

	return true;
}

VCSRigidBody* SketchSolverPoint::vcsBody()
{
	if (NULL != m_pCombinedLine)
		return m_pCombinedLine->vcsBody();
	return SketchSolverGeometry::vcsBody();
}

const AcGePoint3d& SketchSolverPoint::position()
{
	return m_geometry->point();
}

bool SketchSolverPoint::addOnPlaneConstraint(SketchVCSConstraintData& data)
{
	// Add on-plane constraint for sketch point (also for curve's end points)
	// There has an old defect in VCS which generates incorrect DOF sometimes, 
	// a good workaround is adding on-plane constraint for line's end points, 
	// which reduces DOF (could improve performance) during dragBeginSolve.
	data.resetData(this, NULL, 0.0, eOnplaneCons);
	bool res = m_pContext->vcsInterface()->onPlaneConstraint(data);
	NEUTRON_ASSERT(res);
	return res;	
}

bool SketchSolverPoint::degenerate()
{
	return false;
}

bool SketchSolverPoint::updateValueToLastSolved()
{
	if (m_geometry->point().isEqualTo(m_position))
		return true;

	m_pContext->addUpdatedPoint(m_geometry);
	m_geometry->point(m_position);
	return true;
}

VCSRigidBody* SketchSolverPoint::getConstructBodyHorizontal()
{
	if (NULL != m_pConsBodyHorizontal)
		return m_pConsBodyHorizontal;

	return m_pContext->vcsInterface()->createConstructBodyHorizontal(m_pContext->consData(), this);
}

VCSRigidBody* SketchSolverPoint::getConstructBodyVertical()
{
	return NULL;
}

bool SketchSolverPoint::useCombinedBody() const
{
	if (NULL != m_pCombinedLine)
		return true;
	return false;
}

void SketchSolverPoint::useCombinedBody(SketchSolverGeometry* pGeom)
{
	m_pCombinedLine = pGeom;
}

PassiveRef<Geometry3D> SketchSolverPoint::geometry()
{
    return m_geometry;
}

void SketchSolverPoint::updateCachedValue()
{
	m_position = m_geometry->point();
}
