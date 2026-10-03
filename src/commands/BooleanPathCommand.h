#pragma once

#include <QUndoCommand>
#include <QList>
#include <QJsonObject>

class CanvasScene;
class UIComponent;
class PathComponent;

/**
 * BooleanPathCommand
 *
 * Replaces a set of selected shapes with one new multi-contour PathComponent
 * that is the result of a Boolean operation (Union / Subtract / Intersect / XOR).
 *
 * Boolean ops run on the FLATTENED polygon approximation of each input shape
 * (reuses PathComponent::flattenedContours / flattenAnchors on non-path types).
 * True curve-preserving booleans are out of scope.
 *
 * The Clipper2 library is vendored via CMake FetchContent (see CMakeLists.txt).
 */
class BooleanPathCommand : public QUndoCommand {
public:
    enum class Operation { Union, Subtract, Intersect, Xor };

    BooleanPathCommand(CanvasScene* scene,
                       const QList<UIComponent*>& inputs,
                       Operation op,
                       QUndoCommand* parent = nullptr);
    ~BooleanPathCommand() override;

    void undo() override;
    void redo() override;

    /// Static helper: perform the operation and return the result path,
    /// or nullptr if the operation produced an empty result.
    /// Caller owns the returned object.
    static PathComponent* compute(const QList<UIComponent*>& inputs, Operation op);

    /// Returns true when the constructor succeeded and the result is non-empty.
    bool isValid() const { return m_result != nullptr; }

private:
    CanvasScene* m_scene = nullptr;
    QList<UIComponent*> m_inputs;
    PathComponent* m_result = nullptr;
    bool m_ownsResult = false;
    bool m_ownsInputs = false;
    Operation m_op;
};
