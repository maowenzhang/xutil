#include "Pch/SketchPch.h"
#include "SketchVCSExtCurve.h"
#include <Server/Component/Assembly/VCSConv.h>

using namespace SKs::Constraint;

/// ----------------------------------------------------------------------
/// SketchSolverReactor
/// ----------------------------------------------------------------------

SketchVCSExtCurve::SketchVCSExtCurve(const AcGeCurve3d& curve)
{
	m_pCurve = (AcGeCurve3d*)(curve.copy());
	m_bClosed = m_pCurve->isClosed();
}

SketchVCSExtCurve::~SketchVCSExtCurve()
{
	delete m_pCurve;
	m_pCurve = NULL;
}

///< Evaluate point, and derivatives on curve at t.
void SketchVCSExtCurve::evaluate(bool inBoundary)
{
	// Get the U Bounds and insure that param remains within those bounds.
	//
	AcGeInterval uBounds;
	m_pCurve->getInterval(uBounds);
	double tVal = t();

	if (m_bClosed) {
		double len = uBounds.upperBound() - uBounds.lowerBound();
		double val = fmod(tVal, len);
		if (val < uBounds.lowerBound()) 
			tVal = val + len;
		else if( val > uBounds.upperBound()) 
			tVal = val - len;
		else 
			tVal = val;
	}
	else if (inBoundary) {
		if (uBounds.isBoundedBelow() && tVal < uBounds.lowerBound())
			tVal = uBounds.lowerBound();
		if (uBounds.isBoundedAbove() && tVal > uBounds.upperBound())
			tVal = uBounds.upperBound();
	} 

	AcGeVector3dArray tempDerivatives(2);
	AcGePoint3d point = m_pCurve->evalPoint(tVal, 2, tempDerivatives);
	mPointOnCurve = GeConv::toVCSMPoint3d(point);
	mDerivatives[0] = GeConv::toVCSMVector3d(tempDerivatives[0]);
	mDerivatives[1] = GeConv::toVCSMVector3d(tempDerivatives[1]);
}

///< Evaluate point and first-order derivative on this curve at t
void SketchVCSExtCurve::evaluate(double t,  ///< t
								 VCSMPoint3d&    pt, ///< [out] the point on this curve at t.
								 VCSMVector3d&   d   ///< [out] the first-order derivative at t.
								)					
{
	AcGeVector3dArray tempDerivatives(1);
	AcGePoint3d point = m_pCurve->evalPoint(t, 1, tempDerivatives);
	pt = GeConv::toVCSMPoint3d(point);
	d = GeConv::toVCSMVector3d(tempDerivatives[0]);
}

///< Evaluate point on curve at t.
VCSMPoint3d SketchVCSExtCurve::evaluatePoint(double t)
{
	return GeConv::toVCSMPoint3d(m_pCurve->evalPoint(t));
}

///< Get t boundary of the curve.
void SketchVCSExtCurve::getBounds(double& t0,        ///< [out] lower bound of t
								  double& t1         ///< [out] upper bound of t
								  )
{
	AcGeInterval tBounds;
	m_pCurve->getInterval(tBounds);
	if (tBounds.isBounded()) {
		t0 = tBounds.lowerBound();
		t1 = tBounds.upperBound();
	}
	else {
		t0 = VCSLargeNumber;
		t1 = VCSLargeNumber;
	}
}


/// Transform this curve by the input matrix m.
void SketchVCSExtCurve::transformBy(const VCSMMatrix3d&  m)
{
	AcGeMatrix3d geT = GeConv::toAcGeMatrix3d(m);
	m_pCurve->transformBy(geT);
}

void SketchVCSExtCurve::init()
{
	// Get "good" initial conditions for constraint solving.
	//
	AcGePoint3d pointOnCurve = m_pCurve->closestPointTo(GeConv::toAcGePoint3d(mPointOnCurve));
	mT = m_pCurve->paramOf(pointOnCurve);
	evaluate(false);
}

void SketchVCSExtCurve::updateCurve(AcGeCurve3d& newCurve)
{
	delete m_pCurve;
	m_pCurve = (AcGeCurve3d*)(newCurve.copy());
}