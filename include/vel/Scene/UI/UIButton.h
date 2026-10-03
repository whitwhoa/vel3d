#pragma once

#include <string>
#include <functional>

#include <vel/Scene/UI/UIElement.h>
#include <vel/Scene/UI/UICursor.h>

namespace vel
{
    class UIButton : public UIElement
    {
    private:
        friend class Scene;

        bool isMouseOver;
        bool initialMouseOver;
        bool isMouseOut;
        bool initialMouseOut;
        bool isMouseUp;
        bool initialMouseUp;
        bool isMouseDown;
        bool initialMouseDown;
        bool holdUntilMouseUp;

        std::function<void()> mouseOverEvent;
        std::function<void()> mouseOutEvent;
        std::function<void()> mouseUpEvent;
        std::function<void()> mouseDownEvent;

        void setMouseOver(bool b);
        void setMouseDown(bool b);

    protected:
        int			    width;
        int			    height;
        actor_handle    buttonActor;
        std::vector<std::pair<std::string, bool>> backgroundImages;

    public:
        UIButton(const std::string& id);

        virtual UIButton* setMouseOverEvent(std::function<void()> f);
        virtual UIButton* setMouseOutEvent(std::function<void()> f);
        virtual UIButton* setMouseUpEvent(std::function<void()> f);
        virtual UIButton* setMouseDownEvent(std::function<void()> f);
        virtual UIButton* setPosition(glm::vec2 pos) override;
        virtual UIButton* setZIndex(float z) override;
        virtual UIButton* setVisible(bool b) override;
        virtual UIButton* setWidth(int w);
        virtual UIButton* setHeight(int h);
        virtual UIButton* addBackgroundImage(const std::string& i, bool filter = true);
        virtual UIButton* setHoldUntilMouseUp(bool b);

        virtual bool		    containsPoint(glm::vec2 p) override;
        virtual void		    update(float dt, const InputState& is, UICursor* c) override;

        virtual int			    getWidth() const override;
        virtual int			    getHeight() const override;

        bool                    getIsMouseOver() const;
        bool                    isMouseOverSet() const;
        bool                    isMouseOutSet() const;
        bool                    isMouseUpSet() const;
        bool                    isMouseDownSet() const;

        void                    setBackgroundImageIndex(int i);
    };
}