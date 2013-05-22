#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchVCSExtCurve
/// ----------------------------------------------------------------------

// Define _MAC for including VCS headers
#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include <VCSExt.h> // for VCSExtCurve

class VCSMPoint3d;

namespace SKs { namespace Constraint {
	
	/// This class is used to represent VCS extension curve used in VCS solving. 
	/// Used to represent curves used in VCS solving for spline
	/// 
	class SketchVCSExtCurve : public VCSExtCurve
	{
	public:
		SketchVCSExtCurve(const AcGeCurve3d& curve);
		virtual ~SketchVCSExtCurve();

		/// Methods from base class
		///
		virtual void           evaluate(bool inBoundary		///< enforce boundary?
										);					///< Evaluate point, and derivatives on curve at t.
		virtual void           evaluate(double          t,  ///< t
										VCSMPoint3d&    pt, ///< [out] the point on this curve at t.
										VCSMVector3d&   d   ///< [out] the first-order derivative at t.
										);					///< Evaluate point and first-order derivative on this curve at t
		virtual VCSMPoint3d    evaluatePoint(double t);		///< Evaluate point on curve at t.
		virtual void           getBounds(double& t0,        ///< [out] lower bound of t
										 double& t1         ///< [out] upper bound of t
										 );		        ///< Get t boundary of the curve.
		virtual void           transformBy(const VCSMMatrix3d&  m);     ///< Transform this curve by the input matrix m.
		virtual void           init();                                  ///< Initialize.


		void updateCurve(AcGeCurve3d& newCurve);
		VCSMPoint3d&	getNearPoint()  { return mPointOnCurve; }
		const AcGeCurve3d* getAcGeCurve3d() const { return m_pCurve; }

	private:
		/// Current we don't need copy and assignment constructors
		SketchVCSExtCurve(const SketchVCSExtCurve&);
		SketchVCSExtCurve& operator=(const SketchVCSExtCurve&);

		AcGeCurve3d*        m_pCurve;
		bool				m_bClosed;
	};
}}
