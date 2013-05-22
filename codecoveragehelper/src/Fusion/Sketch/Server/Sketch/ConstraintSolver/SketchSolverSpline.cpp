#include "Pch/SketchPch.h"
#include "SketchSolverSpline.h"
#include "SketchSolverLine.h"
#include "SketchSolverCircArc.h"
#include "SketchSolverEllipticArc.h"
#include "SketchSolverPoint.h"
#include "SketchConstraintSolverContext.h"
#include "SketchVCSInterface.h"
#include "SketchVCSConstraintData.h"
#include "SketchVCSDraggingData.h"
#include "SketchVCSExtCurve.h"
#include "SketchVCSExtRigidTester.h"
#include "SketchSolverReactor.h"
#include "../Objects/SketchGeomConstraint.h"

#include <VCSAPI.h>
#include <gemat3d.h>
#include <getol.h>
#include <Server/Component/Assembly/VCSConv.h>
#include <Server/Base/Assert/Assert.h>

// Geometries
#include <Server/Geometry/Geometry3D/Geometry3D.h>
#include <Server/Geometry/Geometry3D/Point3D.h>
#include <Server/Geometry/Geometry3D/TangentHandle.h>
#include <Server/Geometry/Geometry3D/CurvatureHandle.h>
#include <Server/Geometry/Geometry3D/EllipticArc3D.h>
#include <Fusion/Sketch/Server/Sketch/Objects/ConstrainedSpline3D.h>

using namespace Ns;
using namespace SKs::Constraint;
using namespace Ns::Geometry;

/// ----------------------------------------------------------------------
/// SketchSolverSpline
/// ----------------------------------------------------------------------

SketchSolverSpline::SketchSolverSpline(PassiveRef<Spline3D> geometry, SketchConstraintSolverContext* pContext)
	: SketchSolverGeometry(pContext, eSolverGeomSpline),
	m_geometry(geometry),
	m_pReactor(NULL),
	m_extCurves(),
	m_bComputed(false),
	m_FitPoints(),
	m_FitPointsValue(),
    m_needReverse(false),
	m_bHasVCSConstraint(false)
{
	if (m_geometry->isFixed())
		vcsBody()->setGrounded(true);

	getAllFitPoints();
    getAllTangents();
    getAllCurvatures();
	createReactor();
	setposition();
}

SketchSolverSpline::~SketchSolverSpline()
{
	// VCS Ext curves is managed by VCS system
	m_extCurves.clear();

	// VCS Reactor is managed by VCS system
	m_pReactor = NULL;
	m_reactors.clear();
	m_FitPoints.clear();
	m_FitPointsValue.clear();
    m_tangents.clear();
    m_tangentsValue.clear();
    m_curvaturesValue.clear();
}

bool SketchSolverSpline::updateValue(const AcGeMatrix3d& mat)
{
	// TODO
	return true;
}

bool SketchSolverSpline::onAddAdditionalConstraints(SketchVCSConstraintData& data)
{
	// Not add constraint between fit points and spline
	return true;
}

bool SketchSolverSpline::addHandlesForDragging(SketchVCSDraggingData& dragData)
{
	dragData.addBodyToDrag(vcsBody());

	return true;
}

bool SketchSolverSpline::addExtraGeomsForDragging(SketchVCSDraggingData& dragData)
{
	// Add fit points 
	BOOST_FOREACH(SketchSolverGeometry* pItem, m_FitPoints)
	{
		dragData.addGeomToDrag(pItem);
	}
	// Add points to drag together as there has an VCS issue about spline, remove this when it's solved
	m_pContext->consData().addCoincidentPointsToDrag(this, dragData);

	BOOST_FOREACH(SketchSolverGeometry* pItem, m_tangents)
	{
		dragData.addGeomToDrag(pItem);
        SketchSolverLine* handle = pItem->solverGeom<SketchSolverLine>(eSolverGeomLine);
        dragData.addGeomToDrag(handle->solverGeomStartPt());
        dragData.addGeomToDrag(handle->solverGeomEndPt());
	}

	BOOST_FOREACH(SketchSolverGeometry* pItem, m_curvatures)
	{
		dragData.addGeomToDrag(pItem);
        /*
        SketchSolverCircArc* handle = pItem->solverGeom<SketchSolverCircArc>(eSolverGeomLine);
		SketchSolverGeometry* pStartPtGeom = m_pContext->solverGeometry(m_geometry->startPoint());
		SketchSolverGeometry* pEndPtGeom = m_pContext->solverGeometry(m_geometry->endPoint());
        dragData.addGeomToDrag(pStartPtGeom);
        dragData.addGeomToDrag(pEndPtGeom);
        */
	}
	return true;
}

bool SketchSolverSpline::degenerate()
{
	return m_geometry->degenerate();
}

template<typename T>
Spline3D* __getSplineFromHandle(T* handle)
{
    if(handle == 0)
    {
        return 0;
    }
    std::vector<PassiveRef<Curve3D> >& curves = handle->curveUses()->curves();
    BOOST_FOREACH(PassiveRef<Curve3D> c, curves)
    {
        return c->query<Spline3D>();
    }
    return 0;
}

int SketchSolverSpline::onReactorNotify(int iVCSMessage)
{
	// Skip if not the right message
	VCSMessage eVcsMessage = (VCSMessage)iVCSMessage;
	if (kInitialize == eVcsMessage)
	{
		m_bComputed = false;
		return 0;
	}
	if (eVcsMessage != kFullySolved
		&& eVcsMessage != kFullySolvedChanged)
		return 1;
	
	if (m_bComputed)
		return 1;
	m_bComputed = true;

	// Update the ext curve
	AcGeNurbCurve3d newCurve;
	if (generateNewCurve(newCurve))
	{
        if(m_needReverse)
        {
            newCurve.reverseParam();
        }
		BOOST_FOREACH(SketchVCSExtCurve* pExtCurve, m_extCurves)
		{
			pExtCurve->updateCurve(newCurve);
			VCSMMatrix3d mat = vcsBody()->transform().inverse();
			pExtCurve->transformBy(mat);
			VCSMPoint3d bias = pExtCurve->getNearPoint();
			pExtCurve->setPointOnCurve(mat * bias);
			pExtCurve->init();
		}
	}	

	// Form supper body for spline and fits points, both when solving and dragging
	// Put the curve body as the first member of "allBodies". As inside VCS, super body is the representative of its first child. 
	// VCS will set the super body's transform as the first child's transform after it's created.
	VCSCollection allBodies;	
	allBodies.add(vcsBody());
	BOOST_FOREACH(SketchSolverGeometry* pItem, m_FitPoints)
	{
		allBodies.add(pItem->vcsBody());
//        allBodies.add(pItem->vcsBody()->getOwningBody(vcsBody()->root()));
	}

    /*
	BOOST_FOREACH(SketchSolverGeometry* pItem, m_tangents)
	{
		allBodies.add(pItem->vcsBody());
	}
    */

	// Directly return if spline is fixed (or it crashes as an issue when solving fixed spline with rigidset)
	if (m_geometry->isFixed())
		return 1;

	// Work around to only formRigidSet if has vcs constraint on it (coincident, tangent, etc.)
	// To fix issue (fail to drag when 2 fit points are symmetry)
	if (m_bHasVCSConstraint)
		m_pContext->vcsInterface()->vcsSystem()->formRigidSet(allBodies);
	
    bool isChangingSpline = false;
	if (m_pContext->isDragging() && m_pContext->dragData().IsDragSingleGeom())
    {
		SketchSolverGeometry* pDraggedGeom = m_pContext->dragData().geometriesToDrag().back();
        PassiveRef<Geometry3D> geom = pDraggedGeom->geometry();
        if(geom->geomType() == Geometry3D::ePoint3D)
        {
            Point3D* pt = geom->query<Point3D>();
            const StrongRef<CurveUses>& uses = pt->curveUses();
            BOOST_FOREACH(PassiveRef<Curve3D> c, uses->curves())
            {
                if(c.getTarget() == m_geometry.getTarget())
                {
                    isChangingSpline = true;
                    break;
                }

                if(c->geomType() == Geometry3D::eTangentHandle)
                {
                    if(m_geometry.getTarget() == __getSplineFromHandle(c.getTarget()->query<TangentHandle>()))
                    {
                        isChangingSpline = true;
                        break;
                    }
                }
                if(c->geomType() == Geometry3D::eCurvatureHandle)
                {
                    if(m_geometry.getTarget() == __getSplineFromHandle(c.getTarget()->query<CurvatureHandle>()))
                    {
                        isChangingSpline = true;
                        break;
                    }
                }
            }
        }
        else if(geom->geomType() == Geometry3D::eTangentHandle)
        {
            TangentHandle* handle = geom->query<TangentHandle>();
            if(m_geometry.getTarget() == __getSplineFromHandle(handle))
            {
                isChangingSpline = true;
            }
        }
        else if(geom->geomType() == Geometry3D::eCurvatureHandle)
        {
            CurvatureHandle* handle = geom->query<CurvatureHandle>();
            if(m_geometry.getTarget() == __getSplineFromHandle(handle))
            {
                isChangingSpline = true;
            }
        }
    }
	// Set as grounded if fit point or any handle is being dragged
	if (isChangingSpline)
	{
        vcsBody()->parentBody()->setGrounded(true);
	}

	return 1;
}

void SketchSolverSpline::createReactor()
{
	// Create surrogateBody and attach it
	m_pReactor = new SketchSolverReactor(this, SketchSolverReactor::eSpline);
	m_reactors.push_back(m_pReactor);
	VCSRigidBody* pSurrogateBody = m_pContext->vcsInterface()->createRBody();
	pSurrogateBody->setID(reinterpret_cast<UINT_PTR>(pSurrogateBody));
	m_pContext->vcsInterface()->vcsSystem()->attachBodyToReactor(m_pReactor, pSurrogateBody);

	// Set parametric dependency
	BOOST_FOREACH(SketchSolverGeometry* pItem, m_FitPoints)
	{
		pSurrogateBody->setParametricDependency(pItem->vcsBody());
	}
    /*
	BOOST_FOREACH(SketchSolverGeometry* pItem, m_tangents)
	{
		pSurrogateBody->setParametricDependency(pItem->vcsBody());
	}
    */
	
	// This prevents the constraints on curve body be solved before the curve is computed
	vcsBody()->setSolvePreference(kVCSExternallySolved);
}

void SketchSolverSpline::addExtraHandlesForSolving(SketchVCSConstraintData& data, SKs::CurveContinuityConstraint* c)
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

    Spline3D* sp1 = geoms[0]->query<Spline3D>();
    Spline3D* sp2 = geoms[1]->query<Spline3D>();

    int i = c->connectedAtStart1() ? 0 : sp1->fitPoints() - 1;
    const PassiveRef<TangentHandle> t1 = const_cast<TangentHandle*>(sp1->tangentUses()->tangentAt(i));

    i = c->connectedAtStart2() ? 0 : sp2->fitPoints() - 1;
    const PassiveRef<TangentHandle> t2 = const_cast<TangentHandle*>(sp2->tangentUses()->tangentAt(i));

    if(t1 && t2)
    {
        SketchSolverGeometry* l1 = m_pContext->solverGeometry(t1);
        SketchSolverGeometry* l2 = m_pContext->solverGeometry(t2);
        data.resetData(l1, l2, 0, SKs::eParallel, false, true);
        m_pContext->vcsInterface()->parallelConstraint(data);
    }
}

AcGeCircArc3d SketchSolverSpline::updatedCircArc(const PassiveRef<Ns::Geometry::Curve3D>& curve, bool geometryPosition) const
{
    AcGePoint3d c, startPt, endPt;
    AcGeVector3d n;
    double r = 0;
    if(!geometryPosition)
    {
        SketchSolverCircArc* arc = m_pContext->solverGeometry(curve)->solverGeom<SketchSolverCircArc>(eSolverGeomCirArc);

        VCSMCircle3d vcsCir = arc->variableCircleHandle()->circle3d();
        c = GeConv::toAcGePoint3d(vcsCir.center());
        n = GeConv::toAcGeVector3d(vcsCir.normal());
        r = vcsCir.radius();

        if(arc->startPoint() == 0 || arc->endPoint() == 0)
        {
            return AcGeCircArc3d(c, n, r);
        }
        SketchSolverPoint* start = arc->startPoint()->solverGeom<SketchSolverPoint>(eSolverGeomPoint);
        startPt = start->updatedPosition();
        SketchSolverPoint* end = arc->endPoint()->solverGeom<SketchSolverPoint>(eSolverGeomPoint);
        endPt = end->updatedPosition();
    }
    else
    {
        CircArc3D* arc = curve->query<CircArc3D>();
        if(arc->startPoint().isNull() || arc->endPoint().isNull())
        {
            /*
            wchar_t str[1024];
            swprintf(str, L"%f, %f, %f\n", arc->centerPoint()->point().x, arc->centerPoint()->point().y, arc->centerPoint()->point().z);
            OutputDebugStringW(str);
            */
            return AcGeCircArc3d(arc->centerPoint()->point(), arc->normal(), arc->radius());
        }
        c = arc->centerPoint()->point();
        n = arc->normal();
        startPt = arc->startPoint()->point();
        endPt = arc->endPoint()->point();
        r = arc->radius();
    }

    AcGeVector3d v0 = startPt - c;
    AcGeVector3d v1 = endPt - c;
    double endAngle = 0.0f;
    AcGeCircArc3d arc3d;
    if (!v0.isZeroLength())
    {
        endAngle = v0.angleTo(v1, n);
        v0.normalize();
        arc3d = AcGeCircArc3d(c, n, v0, r, 0.0, endAngle);
    }
    else
    {
        double minimumRadius = 10e-5;
        arc3d = AcGeCircArc3d(c, n, minimumRadius);
    }
    return arc3d;
}

AcGeEllipArc3d SketchSolverSpline::updatedEllipArc(const PassiveRef<Ns::Geometry::Curve3D>& curve, bool geometryPosition) const
{
    AcGePoint3d c, startPt, endPt;
    AcGeVector3d majorAxis, minorAxis;
    double majorRadius = 0;
    double minorRadius = 0;
    if(!geometryPosition)
    {
        SketchSolverEllipticArc* arc = m_pContext->solverGeometry(curve)->solverGeom<SketchSolverEllipticArc>(eSolverGeomEllipticArc);
        VCSMEllipse3d vcsCir = arc->variableEllipseHandle()->ellipse3d();
        c = GeConv::toAcGePoint3d(vcsCir.center());
        majorRadius = vcsCir.majorRadius();
        minorRadius = vcsCir.minorRadius();
        majorAxis = GeConv::toAcGeVector3d(vcsCir.majorAxis());
        minorAxis = GeConv::toAcGeVector3d(vcsCir.minorAxis());
        SketchSolverPoint* start = arc->startPoint()->solverGeom<SketchSolverPoint>(eSolverGeomPoint);
        startPt = start->updatedPosition();
        SketchSolverPoint* end = arc->endPoint()->solverGeom<SketchSolverPoint>(eSolverGeomPoint);
        endPt = end->updatedPosition();
    }
    else
    {
        EllipticArc3D* arc = curve->query<EllipticArc3D>();
        AcGeEllipArc3d updatedCurve = arc->arcCurve();
		majorAxis = updatedCurve.majorAxis() * updatedCurve.majorRadius();
		minorAxis = updatedCurve.minorAxis() * updatedCurve.minorRadius();
        majorRadius = updatedCurve.majorRadius();
        minorRadius = updatedCurve.minorRadius();
        startPt = arc->startPoint()->point();
        endPt = arc->endPoint()->point();
        c = arc->centerPoint()->point();
    }

    double startAngle = EllipticArc3D::decomposePointIntoAngParam(c, majorAxis, minorAxis, startPt);
    double endAngle = EllipticArc3D::decomposePointIntoAngParam(c, majorAxis, minorAxis, endPt);

    AcGeEllipArc3d arc3d(c, majorAxis, minorAxis, majorRadius, minorRadius, startAngle, endAngle);
    return arc3d;
}

AcGeLineSeg3d SketchSolverSpline::updatedLineSeg(const PassiveRef<Ns::Geometry::Curve3D>& curve, bool geometryPosition) const
{
    AcGePoint3d startPt, endPt;
    if(!geometryPosition)
    {
        SketchSolverLine* line = m_pContext->solverGeometry(curve)->solverGeom<SketchSolverLine>(eSolverGeomLine);
        SketchSolverPoint* start = line->solverGeomStartPt()->solverGeom<SketchSolverPoint>(eSolverGeomPoint);
        startPt = start->updatedPosition();
        SketchSolverPoint* end = line->solverGeomEndPt()->solverGeom<SketchSolverPoint>(eSolverGeomPoint);
        endPt = end->updatedPosition();
    }
    else
    {
        Line3D* line = curve->query<Line3D>();
        startPt = line->startPoint()->point();
        endPt = line->endPoint()->point();
    }

    AcGeLineSeg3d arc3d(startPt, endPt);
    return arc3d;
}

AcGeNurbCurve3d SketchSolverSpline::computeSpline(std::vector<AcGePoint3d>& fitPts, bool geometryPosition)
{
    AcGeNurbCurve3d ret;
    bool bStartContinuityCurveAtStart;
    bool bStartTangentCurveOutward;
    bool bEndContinuityCurveAtStart;
    bool bEndTangentCurveOutward;

    Spline3D::EContinuityFlag startContinuity;
    Spline3D::EContinuityFlag endContinuity;
    PassiveRef<Curve3D>	rStartContinuityCurve = m_geometry->startContinuityCurve(bStartContinuityCurveAtStart, bStartTangentCurveOutward, startContinuity);
    PassiveRef<Curve3D> rEndContinuityCurve = m_geometry->endContinuityCurve(bEndContinuityCurveAtStart, bEndTangentCurveOutward, endContinuity);

    AcGeVector3d* pStartTangent = 0;
    AcGeVector3d* pStartCurvature = 0;
    AcGeVector3d* pEndTangent = 0;
    AcGeVector3d* pEndCurvature = 0;

    AcGeVector3d startTangent;
    AcGeVector3d startCurvature;
    AcGeVector3d endTangent;
    AcGeVector3d endCurvature;
    if(rStartContinuityCurve.isNotNull())
    {
        Ns::PassiveRef<Geometry::ConstrainedSpline3D> sp = m_geometry->query<Geometry::ConstrainedSpline3D>();
        if(rStartContinuityCurve->geomType() == Geometry3D::eCircArc3D)
        {
            AcGeCircArc3d arc3d = updatedCircArc(rStartContinuityCurve, geometryPosition);
            if(startContinuity & Spline3D::eCurvatureContinuity)
            {
                startTangent = ConstrainedSpline3D::computeContinuityVector(sp, arc3d, bStartContinuityCurveAtStart, bStartTangentCurveOutward, 0, (Ns::uint8)Spline3D::eTangentContinuity, false);
                pStartTangent = &startTangent;
            }
            startCurvature = ConstrainedSpline3D::computeContinuityVector(sp, arc3d, bStartContinuityCurveAtStart, bStartTangentCurveOutward, 0, startContinuity, true);
            pStartCurvature = &startCurvature;
        }
        else if(rStartContinuityCurve->geomType() == Geometry3D::eEllipticArc3D)
        {
            AcGeEllipArc3d arc3d = updatedEllipArc(rStartContinuityCurve, geometryPosition);
            if(startContinuity & Spline3D::eCurvatureContinuity)
            {
                startTangent = ConstrainedSpline3D::computeContinuityVector(sp, arc3d, bStartContinuityCurveAtStart, bStartTangentCurveOutward, 0, (Ns::uint8)Spline3D::eTangentContinuity, false);
                pStartTangent = &startTangent;
            }
            startCurvature = ConstrainedSpline3D::computeContinuityVector(sp, arc3d, bStartContinuityCurveAtStart, bStartTangentCurveOutward, 0, startContinuity, true);
            pStartCurvature = &startCurvature;
        }
        else if(rStartContinuityCurve->geomType() == Geometry3D::eLine3D)
        {
            AcGeLineSeg3d l = updatedLineSeg(rStartContinuityCurve, geometryPosition);
            if(startContinuity & Spline3D::eCurvatureContinuity)
            {
                startTangent = ConstrainedSpline3D::computeContinuityVector(sp, l, bStartContinuityCurveAtStart, bStartTangentCurveOutward, 0, (Ns::uint8)Spline3D::eTangentContinuity, false);
                pStartTangent = &startTangent;
            }
            startCurvature = ConstrainedSpline3D::computeContinuityVector(sp, l, bStartContinuityCurveAtStart, bStartTangentCurveOutward, 0, startContinuity, true);
            pStartCurvature = &startCurvature;
        }
    }
    if(rEndContinuityCurve.isNotNull())
    {
        Ns::PassiveRef<Geometry::ConstrainedSpline3D> sp = m_geometry->query<Geometry::ConstrainedSpline3D>();
        int id = sp->fitPoints() - 1;
        if(rEndContinuityCurve->geomType() == Geometry3D::eCircArc3D)
        {
            AcGeCircArc3d arc3d = updatedCircArc(rEndContinuityCurve, geometryPosition);
            if(endContinuity & Spline3D::eCurvatureContinuity)
            {
                endTangent = ConstrainedSpline3D::computeContinuityVector(sp, arc3d, bEndContinuityCurveAtStart, bEndTangentCurveOutward, id, (Ns::uint8)Spline3D::eTangentContinuity, false);
                pEndTangent = &endTangent;
            }
            endCurvature = ConstrainedSpline3D::computeContinuityVector(sp, arc3d, bEndContinuityCurveAtStart, bEndTangentCurveOutward, id, endContinuity, true);
            pEndCurvature = &endCurvature;
        }
        else if(rEndContinuityCurve->geomType() == Geometry3D::eEllipticArc3D)
        {
            AcGeEllipArc3d arc3d = updatedEllipArc(rEndContinuityCurve, geometryPosition);
            if(endContinuity & Spline3D::eCurvatureContinuity)
            {
                endTangent = ConstrainedSpline3D::computeContinuityVector(sp, arc3d, bEndContinuityCurveAtStart, bEndTangentCurveOutward, id, (Ns::uint8)Spline3D::eTangentContinuity, false);
                pEndTangent = &endTangent;
            }
            endCurvature = ConstrainedSpline3D::computeContinuityVector(sp, arc3d, bEndContinuityCurveAtStart, bEndTangentCurveOutward, id, endContinuity, true);
            pEndCurvature = &endCurvature;
        }
        else if(rEndContinuityCurve->geomType() == Geometry3D::eLine3D)
        {
            AcGeLineSeg3d line = updatedLineSeg(rEndContinuityCurve, geometryPosition);
            if(endContinuity & Spline3D::eCurvatureContinuity)
            {
                endTangent = ConstrainedSpline3D::computeContinuityVector(sp, line, bEndContinuityCurveAtStart, bEndTangentCurveOutward, id, (Ns::uint8)Spline3D::eTangentContinuity, false);
                pEndTangent = &endTangent;
            }
            endCurvature = ConstrainedSpline3D::computeContinuityVector(sp, line, bEndContinuityCurveAtStart, bEndTangentCurveOutward, id, endContinuity, true);
            pEndCurvature = &endCurvature;
        }
    }

    std::map<int, AcGeVector3d> tangents;
    std::map<int, AcGeVector3d> curvatures;
    if(m_geometry->tangentUses().isNotNull())
    {
        for(std::map<int, StrongRef<TangentHandle> >::const_iterator it = m_geometry->tangentUses()->tangents().begin(); 
            it != m_geometry->tangentUses()->tangents().end(); ++it)
        {
            AcGeLineSeg3d line = updatedLineSeg(it->second, geometryPosition);
            tangents.insert(std::pair<int, AcGeVector3d>(it->first, (line.endPoint() - line.startPoint()) / 2));
        }
    }

    if(m_geometry->curvatureUses().isNotNull())
    {
        for(std::map<int, StrongRef<CurvatureHandle> >::const_iterator it = m_geometry->curvatureUses()->curvatures().begin(); 
            it != m_geometry->curvatureUses()->curvatures().end(); ++it)
        {
            curvatures.insert(std::pair<int, AcGeVector3d>(it->first, it->second->curvature()));
        }
    }
    // Generate new spline
    m_geometry->generateNewCurve(fitPts, tangents, curvatures, ret, pStartTangent, pStartCurvature, pEndTangent, pEndCurvature);
    return ret;
}

/// Generate new curve based on latest changes, below logic should align with the updates in spline3d
bool SketchSolverSpline::generateNewCurve(AcGeNurbCurve3d& newCurve)
{
    // Get updated fit points
    std::vector<AcGePoint3d> fitPts;
    bool bHasFitPointChanged = false;
    for (size_t i=0; i<m_FitPoints.size(); i++)
    {
        AcGePoint3d& cachedPt = m_FitPointsValue[i];
        AcGePoint3d pt = m_FitPoints[i]->updatedPosition();
        if (!pt.isEqualTo(cachedPt))
            bHasFitPointChanged = true;

        fitPts.push_back(pt);
    }
    if (!bHasFitPointChanged)
        return false;

    newCurve = computeSpline(fitPts);
    return true;
}

void SketchSolverSpline::getAllFitPoints()
{
	// All internal fit points
	std::vector< PassiveRef<Point3D> > fitPoints = m_geometry->deleteNominees();
	BOOST_FOREACH(PassiveRef<Point3D>& pt, fitPoints)
	{
		SketchSolverGeometry* pGeom = m_pContext->solverGeometry(pt);
		AcGePoint3d gept = pGeom->updatedPosition();
		m_FitPoints.push_back(pGeom);
		m_FitPointsValue.push_back(gept);
	}
}

void SketchSolverSpline::getAllTangents()
{
    const StrongRef<TangentUses>& uses = m_geometry->tangentUses();
    if(uses.isNull())
    {
        return;
    }
    for(std::map<int, StrongRef<TangentHandle> >::iterator it = uses->tangents().begin(); it != uses->tangents().end(); ++it)
    {
		SketchSolverGeometry* pGeom = m_pContext->solverGeometry(it->second);
        m_tangents.push_back(pGeom);
		m_tangentsValue.insert(std::pair<int, AcGeVector3d>(it->first, (it->second)->tangent()));
    }
}

void SketchSolverSpline::getAllCurvatures()
{
    const StrongRef<CurvatureUses>& uses = m_geometry->curvatureUses();
    if(uses.isNull())
    {
        return;
    }
    for(std::map<int, StrongRef<CurvatureHandle> >::iterator it = uses->curvatures().begin(); it != uses->curvatures().end(); ++it)
    {
		SketchSolverGeometry* pGeom = m_pContext->solverGeometry(it->second);
        m_curvatures.push_back(pGeom);
		m_curvaturesValue.insert(std::pair<int, AcGeVector3d>(it->first, (it->second)->curvature()));
    }
}

SketchVCSExtCurve* SketchSolverSpline::extCurve()
{
	// Each matePtCurve constraint should have its own ExtCurve (or crash when its owner delete it)
    AcGeNurbCurve3d crv = m_geometry->splineCurve();
    if(m_needReverse)
    {
        crv.reverseParam();
    }
	SketchVCSExtCurve* pExtCurve = new SketchVCSExtCurve(crv);
	m_extCurves.push_back(pExtCurve);
	return pExtCurve;
}

bool SketchSolverSpline::isExternalRigid()
{
	bool bAllRigid = true;
	BOOST_FOREACH(SketchSolverReactor* pItem, m_reactors)
	{
		if (!pItem->isRigid())
		{
			bAllRigid = false;
			break;
		}
	}
	return bAllRigid;
}

VCSExtGeometry* SketchSolverSpline::extGeometry()
{
	// Not use rigid tester for spline anymore
	return NULL;
}

PassiveRef<Geometry3D> SketchSolverSpline::geometry()
{
    return m_geometry;
}

bool SketchSolverSpline::addHandlesForDraggingRelatedItem(SketchSolverGeometry* pDraggedItem)
{
	return true;
}

bool SketchSolverSpline::addExtraHandlesForDragging(SketchVCSDraggingData& dragData)
{
	return true;
}

void SketchSolverSpline::setposition()
{
	const AcGeNurbCurve3d& tmpCurve = m_geometry->splineCurve();
	AcGePoint3dArray curvePoints;
	tmpCurve.getSamplePoints(3, curvePoints);
	m_position = curvePoints[1];
}

bool SketchSolverSpline::needReverse() const
{
    return m_needReverse;
}

void SketchSolverSpline::needReverse(bool reverse)
{
    m_needReverse = reverse;
}

void SketchSolverSpline::hasVCSConstraint(bool val)
{
	m_bHasVCSConstraint = val;
}