#include <map>
#include <unordered_set>
#include <vector>

#include <ShapeBuild_ReShape.hxx>
#include <Standard_Version.hxx>
#include <TopoDS_Shape.hxx>
#include <TopExp_Explorer.hxx>
#include "TopoShape.h"

class BRepBuilderAPI_MakeShape;
class BRepTools_History;
class BRepTools_ReShape;
class ShapeFix_Root;

namespace Part
{

enum class MappingStatus
{
    Generated,
    Modified
};
/** Shape mapper for user defined shape mapping
 */
struct PartExport ShapeMapper: TopoShape::Mapper
{
    virtual ~ShapeMapper() noexcept = default;

    /** Populate mapping from a source shape to a list of shape
     *
     * @param status: whether the shape is generated
     * @param src: source shape
     * @param dst: a list of sub shapes in the new shape
     *
     * The source will be expanded into sub shapes of faces, edges and vertices
     * before being inserted into the map.
     */
    void populate(MappingStatus status, const TopoShape& src, const TopTools_ListOfShape& dst);
    /** Populate mapping from a source sub shape to a list of shape
     *
     * @param status: whether the shape is generated
     * @param src: a list of sub shapes in the source shape
     * @param dst: a list of sub shapes in the new shape
     *
     * The source will be expanded into sub shapes of faces, edges and vertices
     * before being inserted into the map.
     */
    void populate(MappingStatus status,
                  const TopTools_ListOfShape& src,
                  const TopTools_ListOfShape& dst);

    /** Populate mapping from a source sub shape to a list of shape
     *
     * @param status: whether the shape is generated
     * @param src: a list of sub shapes in the source shape
     * @param dst: a list of sub shapes in the new shape
     *
     * The source will be expanded into sub shapes of faces, edges and vertices
     * before being inserted into the map.
     */
    void populate(MappingStatus status,
                  const std::vector<TopoShape>& src,
                  const std::vector<TopoShape>& dst)
    {
        for (auto& s : src) {
            populate(status, s, dst);
        }
    }

    /** Populate mapping from a source sub shape to a list of shape
     *
     * @param status: whether the shape is generated
     * @param src: a sub shape of the source shape
     * @param dst: a list of sub shapes in the new shape
     *
     * The source will be expanded into sub shapes of faces, edges and vertices
     * before being inserted into the map.
     */
    void populate(MappingStatus status, const TopoShape& src, const std::vector<TopoShape>& dst)
    {
        if (src.isNull()) {
            return;
        }
        std::vector<TopoDS_Shape> dstShapes;
        for (auto& d : dst) {
            expand(d.getShape(), dstShapes);
        }
        insert(status, src.getShape(), dstShapes);
    }

    /** Expand a shape into faces, edges and vertices
     * @params d: shape to expand
     * @param shapes: output sub shapes of faces, edges and vertices
     */
    void expand(const TopoDS_Shape& d, std::vector<TopoDS_Shape>& shapes);

    /** Insert a map entry from a sub shape in the source to a list of sub shapes in the new shape
     *
     * @params status: whether the sub shapes are generated or modified
     * @param s: a sub shape in the source
     * @param d: a list of sub shapes in the new shape
     */
    void insert(MappingStatus status, const TopoDS_Shape& s, const std::vector<TopoDS_Shape>& d);

    /** Insert a map entry from a sub shape in the source to a sub shape in the new shape
     *
     * @params status: whether the sub shapes are generated or modified
     * @param s: a sub shape in the source
     * @param d: a list of sub shapes in the new shape
     */
    void insert(MappingStatus status, const TopoDS_Shape& s, const TopoDS_Shape& d);

    const std::vector<TopoDS_Shape>& generated(const TopoDS_Shape& s) const override
    {
        auto iter = _generated.find(s);
        if (iter != _generated.end()) {
            return iter->second.shapes;
        }
        return _res;
    }

    const std::vector<TopoDS_Shape>& modified(const TopoDS_Shape& s) const override
    {
        auto iter = _modified.find(s);
        if (iter != _modified.end()) {
            return iter->second.shapes;
        }
        return _res;
    }

    std::vector<TopoShape> shapes;
    std::unordered_set<TopoDS_Shape, ShapeHasher, ShapeHasher> shapeSet;

    struct ShapeValue
    {
        std::vector<TopoDS_Shape> shapes;
        std::unordered_set<TopoDS_Shape, ShapeHasher, ShapeHasher> shapeSet;
    };
    typedef std::unordered_map<TopoDS_Shape, ShapeValue, ShapeHasher, ShapeHasher> ShapeMap;
    ShapeMap _generated;
    std::unordered_set<TopoDS_Shape, ShapeHasher, ShapeHasher> _generatedShapes;
    ShapeMap _modified;
    std::unordered_set<TopoDS_Shape, ShapeHasher, ShapeHasher> _modifiedShapes;
};

/// Parameters for TopoShape::makeElementFilledFace()
struct PartExport TopoShape::BRepFillingParams
{
    /** Optional initial surface to begin the construction of the surface for the filled face.
     *
     *  It is useful if the surface resulting from construction for the
     *  algorithm is likely to be complex. The support surface of the face
     *  under construction is computed by a deformation of Surf which satisfies
     *  the given constraints. The set of bounding edges defines the wire of
     *  the face. If no initial surface is given, the algorithm computes it
     *  automatically. If the set of edges is not connected (Free constraint),
     *  missing edges are automatically computed. Important: the initial
     *  surface must have orthogonal local coordinates, i.e. partial
     *  derivatives dS/du and dS/dv must be orthogonal at each point of
     *  surface. If this condition breaks, distortions of resulting surface are
     *  possible
     */
    TopoShape surface;
    /** Optional map from input edge to continutity order. The default
     *  continuity order is TopoShape::Continuity::C0.
     */
    std::unordered_map<TopoDS_Shape, TopoShape::Continuity, ShapeHasher, ShapeHasher> orders;
    /// Optional map from input shape to face used as support
    std::unordered_map<TopoDS_Shape, TopoDS_Shape, ShapeHasher, ShapeHasher> supports;
    /// Optional begin index to the input shapes to be used as the boundary of the filled face.
    int boundary_begin = -1;
    /// Optional end index (last index + 1) to the input shapes to be used as the boundary of the
    /// filled face.
    int boundary_end = -1;
    /// The energe minimizing criterion degree;
    unsigned int degree = 3;
    /// The number of points on the curve NbPntsOnCur
    unsigned int ptsoncurve = 15;
    /// The number of iterations NbIter
    unsigned int numiter = 2;
    /// The Boolean Anisotropie
    bool anisotropy = false;
    /// The 2D tolerance Tol2d
    double tol2d = 1e-5;
    /// The 3D tolerance Tol3d
    double tol3d = 1e-4;
    /// The angular tolerance TolAng
    double tolG1 = 0.01;
    /// The tolerance for curvature TolCur
    double tolG2 = 0.1;
    /// The highest polynomial degree MaxDeg
    unsigned int maxdeg = 8;
    /** The greatest number of segments MaxSeg.
     *
     * If the Boolean Anistropie is true, the algorithm's performance is better
     * in cases where the ratio of the length U and the length V indicate a
     * great difference between the two. In other words, when the surface is,
     * for example, extremely long.
     */
    unsigned int maxseg = 9;
};


}  // namespace Part
