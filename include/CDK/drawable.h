#ifndef __H_DRAWABLE__
#define __H_DRAWABLE__
#include <memory>

#include "CDK\material.h"
#include "CDK\geometry.h"
#include "node.h"
class Drawable :public Node {
  
public:
	struct Data;
	Drawable();
	//Getter and setters�
	vec3 position();
	vec3 rotation();
	vec3 scale();
  
  std::shared_ptr<Geometry> geometry();
  std::shared_ptr<Material> material();

/*	void setPosition(vec3 &data);
	void setPosition(const float* data);

	void setRotation(vec3 &data);
	void setRotation(const float* data);

	void setScale(vec3 &data);
	void setScale(const float* data);*/

	void setGeometry(std::shared_ptr<Geometry>);
	void setMaterial(std::shared_ptr<Material>);

private:
	Data *data_;

};

#endif