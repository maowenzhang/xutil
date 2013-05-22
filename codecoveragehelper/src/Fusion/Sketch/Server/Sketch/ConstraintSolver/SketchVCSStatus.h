#pragma once

/// ----------------------------------------------------------------------
/// This file is used to keep SketchVCSStatus
/// ----------------------------------------------------------------------

#ifdef NEUTRON_OS_MAC
#define _MAC
#endif

#include <VCSAPI.h> // VCSStatus
#include <Server/Base/Globals.h> // Ns::IString

namespace SKs { namespace Constraint {
	
	/// This is used to store status data of vcs solving/dragging
	/// 
	class SketchVCSStatus
	{
	public:
		SketchVCSStatus();
		~SketchVCSStatus();

		void status(VCSStatus& eStatus);
		VCSStatus& status();
		bool isSuccess() const;
		bool isSuccess(VCSStatus& eStatus);
		const Ns::IString& errorMessage() const;
		void cleanup();

	private:
		/// Current we don't need copy and assignment constructors
		SketchVCSStatus(const SketchVCSStatus&);
		SketchVCSStatus& operator=(const SketchVCSStatus&);

		VCSStatus m_eVCSStatus;
	};
}}
