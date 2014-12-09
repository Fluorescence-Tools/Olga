#ifndef TRAJECTORIESTREEITEM_H
#define TRAJECTORIESTREEITEM_H

#include <functional>

struct TrajectoriesTreeItem
{
	int moltrajIndex;
	int trajindex;
	unsigned char nesting() const
	{
		if(moltrajIndex<0){
			return 0;
		}
		else {
			return trajindex<0?1:2;
		}
	}
	bool inline operator==(const TrajectoriesTreeItem& rhs) const {
	    return (moltrajIndex == rhs.moltrajIndex) &&
			    (trajindex==rhs.trajindex);
	}
};
namespace std {
template <>
struct hash<TrajectoriesTreeItem>  {
	size_t operator()(const TrajectoriesTreeItem &itm) const
	{
		return std::hash<int>()(itm.moltrajIndex)^
				std::hash<int>()(itm.trajindex);
	}
};
}
#endif // TRAJECTORIESTREEITEM_H
