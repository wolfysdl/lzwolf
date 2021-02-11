#ifndef __WL_FRAMEDATA_H__
#define __WL_FRAMEDATA_H__

#include <map>
#include <functional>
#include "wl_def.h"
#include "actor.h"

/*
=============================================================================

						WL_FRAMEDATA DEFINITIONS

=============================================================================
*/

struct FrameData
{
	void InitXActors(std::function<bool(AActor *)> reject_cb)
	{
		xactors.clear();
		max_radius = 0;

		for(AActor::Iterator iter = AActor::GetIterator().Next();iter;)
		{
			AActor *check = iter;
			iter.Next();

			if(reject_cb(check))
				continue;

			max_radius = std::max(max_radius, check->radius);
			xactors.insert(std::make_pair(check->x, check));
		}
	}

	std::multimap<fixed, AActor*> xactors;
	fixed max_radius;
};


#endif
