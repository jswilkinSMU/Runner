#include "Game/Entity.hpp"
#include "Engine/Math/Mat44.hpp"

Entity::Entity(Game* owner, Vec3 const& position, EulerAngles orientation, Rgba8 color)
	:m_game(owner),
	 m_position(position),
	 m_orientation(orientation),
	 m_color(color)
{
}

Entity::~Entity()
{
}

Mat44 Entity::GetModelToWorldTransform() const
{
	Mat44 modelToWorldMatrix;
	modelToWorldMatrix.SetTranslation3D(m_position);
	EulerAngles orientation;
	modelToWorldMatrix.Append(orientation.GetAsMatrix_IFwd_JLeft_KUp());
	return modelToWorldMatrix;
}
