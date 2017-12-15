#include <osg/Node>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <fstream>
#include <format_parsers.hpp>

namespace MFFormat
{

class Loader
{
public:
    osg::ref_ptr<osg::Node> load4ds(std::ifstream &srcFile);

protected:
    osg::ref_ptr<osg::Node> make4dsMesh(MFFormat::DataFormat4DS::Mesh *mesh);
    osg::ref_ptr<osg::Node> make4dsMeshLOD(MFFormat::DataFormat4DS::Lod *meshLOD);
};

osg::ref_ptr<osg::Node> Loader::make4dsMesh(DataFormat4DS::Mesh *mesh)
{
    std::cout << "LODs: " << ((int) mesh->mStandard.mLODLevel) << std::endl;

    const float maxDistance = 50.0;
    const float stepLOD = maxDistance / mesh->mStandard.mLODLevel;

    osg::ref_ptr<osg::LOD> nodeLOD = new osg::LOD();

    for (int i = 0; i < mesh->mStandard.mLODLevel; ++i)
    {
        nodeLOD->addChild( make4dsMeshLOD(&(mesh->mStandard.mLODs[i])));
        nodeLOD->setRange(i,i * stepLOD, (i + 1) * stepLOD);
    }

    return nodeLOD; 
}

osg::ref_ptr<osg::Node> Loader::make4dsMeshLOD(DataFormat4DS::Lod *meshLOD)
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> uvs = new osg::Vec2Array;

    for (size_t i = 0; i < meshLOD->mVertexCount; ++i)
    {
        auto vertex = meshLOD->mVertices[i];

        vertices->push_back(osg::Vec3f(vertex.mPos.x, vertex.mPos.y, vertex.mPos.z));
        normals->push_back(osg::Vec3f(vertex.mNormal.x, vertex.mNormal.y, vertex.mNormal.z));
        uvs->push_back(osg::Vec2f(vertex.mUV.x, vertex.mUV.y));
    }

    osg::ref_ptr<osg::DrawElementsUInt> indices = new osg::DrawElementsUInt(GL_TRIANGLES, 6);

    for (size_t i = 0; i < meshLOD->mFaceGroups[0].mFaceCount; ++i)
    {
        auto face = meshLOD->mFaceGroups[0].mFaces[i];
        indices->push_back(face.mA);
        indices->push_back(face.mB);
        indices->push_back(face.mC);
    }

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(vertices.get());
    geom->setNormalArray(normals.get());
    geom->setTexCoordArray(0,uvs.get());

    geom->addPrimitiveSet(indices.get());
    geode->addDrawable(geom.get());

    return geode;
}

osg::ref_ptr<osg::Node> Loader::load4ds(std::ifstream &srcFile)
{
    MFFormat::DataFormat4DS format;

    osg::ref_ptr<osg::Group> group = new osg::Group();

    if (format.load(srcFile))
    {
        auto model = format.getModel();
        
        std::cout << "meshes: " << model->mMeshCount << std::endl;

        for (int i = 0; i < model->mMeshCount; ++i)
        {
            osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform();
            osg::Matrixd mat;

            MFFormat::DataFormat4DS::Vec3 p, s;
            MFFormat::DataFormat4DS::Quat r;

            p = model->mMeshes[i].mPos;
            s = model->mMeshes[i].mScale;
            r = model->mMeshes[i].mRot;

            mat.preMultTranslate(osg::Vec3f(p.x,p.y,p.z));
            mat.preMultScale(osg::Vec3f(s.x,s.y,s.z));
            mat.preMultRotate(osg::Quat(r.x,r.y,r.z,r.w)); 

            transform->setMatrix(mat);

            transform->addChild(make4dsMesh(&(model->mMeshes[i])));
            group->addChild(transform);
        }
    }

    return group;
}

}
