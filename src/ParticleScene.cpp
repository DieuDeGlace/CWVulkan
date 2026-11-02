#include "ParticleScene.h"
#include "Renderer.h"

namespace particlescene {
DrawItem draw(const nativeparticles::Particle &particle, uint32_t mesh, glm::ivec3 dimensions, const Coordinates &coordinates) {
    DrawItem item;
    item.mesh = mesh;
    item.kind = DrawKind::Particle;
    item.castShadow = false;
    item.material = particle.colour;
    item.nativeParticle = scenevisibility::Sphere{particle.position, particle.size};
    // Owner meshes are X/Y-centered; the native matrix addresses voxel corners.
    const glm::vec3 centre(dimensions.x * .5f, dimensions.y * .5f, 0);
    item.model = nativeparticles::matrix(particle, coordinates.homeOffset, dimensions);
    item.model[3].z += coordinates.homeZ;
    item.model = glm::translate(item.model, centre);
    if (coordinates.renderOffset)
        item.nativeModel = glm::translate(nativeparticles::matrix(particle, *coordinates.renderOffset, dimensions), centre);
    return item;
}
} // namespace particlescene
