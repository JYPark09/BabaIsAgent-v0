// Copyright(c) 2019 Junyeong Park, Hyeonsu Kim

#include <Baba/Enums/Game.h>
#include <Baba/Game/Game.h>
#include <Baba/Rules/Effects.h>

#include <algorithm>
#include <stdexcept>

namespace Baba
{
Game::Game(std::size_t width, std::size_t height)
    : width_(width), height_(height), map_(width * height)
{
    // Do nothing
}

Game::~Game()
{
    for (auto& obj : objects_)
    {
        delete obj;
    }
    objects_.clear();
}

std::size_t Game::GetWidth() const
{
    return width_;
}

std::size_t Game::GetHeight() const
{
    return height_;
}

const Object::Arr& Game::At(std::size_t x, std::size_t y) const
{
    return map_[x + y * width_];
}

Object& Game::Put(std::size_t x, std::size_t y)
{
    objects_.emplace_back(new Object);
    map_[x + y * width_].emplace_back(objects_.back());

    return *map_[x + y * width_].back();
}

void Game::DestroyObject(Object& object)
{
    for (auto& objs : map_)
    {
        for (auto obj = objs.begin(); obj != objs.end(); ++obj)
        {
            if (**obj == object)
            {
                (*obj)->Destroy();
                objs.erase(obj);
                return;
            }
        }
    }
}

Object::Arr Game::FindObjects(std::function<bool(const Object&)> func) const
{
    Object::Arr result;

    for (auto& objs : map_)
    {
        for (auto& obj : objs)
        {
            if (func(*obj))
            {
                result.emplace_back(obj);
            }
        }
    }

    return result;
}

Object::Arr Game::FindObjectsByType(ObjectType type) const
{
    return FindObjects(
        [type](const Object& obj) { return obj.GetType() == type; });
}

Object::Arr Game::FindObjectsByProperty(PropertyType property) const
{
    return FindObjects(
        [property](const Object& obj) { return obj.HasProperty(property); });
}

Object::Arr Game::FindObjectsByPosition(const Object& target) const
{
    for (auto& objs : map_)
    {
        for (auto& obj : objs)
        {
            if (*obj == target)
            {
                return objs;
            }
        }
    }

    return Object::Arr();
}

Object::Arr Game::FilterObjectByFunction(
    const Object::Arr& objects, std::function<bool(const Object&)> func) const
{
    Object::Arr result;

    for (auto& obj : objects)
    {
        if (func(*obj))
        {
            result.emplace_back(obj);
        }
    }

    return result;
}

const Game::Point Game::GetPositionByObject(const Object& target) const
{
    for (std::size_t y = 0; y < GetHeight(); y++)
    {
        for (std::size_t x = 0; x < GetWidth(); x++)
        {
            const auto& objs = map_[x + y * width_];

            for (auto& obj : objs)
            {
                if (*obj == target)
                {
                    return { x, y };
                }
            }
        }
    }

    throw std::runtime_error("Invalid target");
}

bool Game::ValidatePosition(std::size_t x, std::size_t y) const
{
    return x < width_ && y < height_;
}

void Game::Update(Action action)
{
    parseRules();
    if (action != Action::STAY)
    {
        // Do nothing
    }

    applyRules();

    determineResult();
}

GameResult Game::GetGameResult() const
{
    return gameResult_;
}

std::int64_t Game::AddRule(ObjectType target, ObjectType verb,
                           ObjectType effect)
{
    rules_.emplace(target, verb, effect);

    return Rule::GetRuleID(target, verb, effect);
}

void Game::DeleteRule(std::int64_t id)
{
    auto it =
        std::find_if(rules_.begin(), rules_.end(),
                     [id](const Rule& rule) { return rule.GetRuleID() == id; });
    if (it != rules_.end())
    {
        rules_.erase(it);
    }
}

void Game::parseRules()
{
    auto verbs = FindObjects(
        [](const Object& obj) { return IsVerbType(obj.GetType()); });

    for (auto& verb : verbs)
    {
        auto [x, y] = GetPositionByObject(*verb);

        const auto addRules = [&, x, y](std::size_t dx, std::size_t dy) {
            if (ValidatePosition(x - dx, y - dy) && ValidatePosition(x + dx, y + dy))
            {
                auto targets = FilterObjectByFunction(
                    At(x - dx, y - dy),
                    [](const Object& obj) { return obj.IsText(); });
                auto effects = FilterObjectByFunction(
                    At(x + dx, y + dy),
                    [](const Object& obj) { return obj.IsText(); });

                for (auto& target : targets)
                {
                    for (auto& effect : effects)
                    {
                        AddRule(target->GetType(), verb->GetType(),
                                effect->GetType());
                    }
                }
            }
        };

        addRules(1, 0);
        addRules(0, 1);
    }
}

void Game::applyRules()
{
    auto& effects = Effects::GetInstance().GetEffects();

    for (auto& rule : rules_)
    {
        if (rule.GetVerb() == ObjectType::IS)
        {
            auto targets = FindObjectsByType(rule.GetTarget());

            if (IsObjectType(rule.GetEffect()))
            {
                for (auto& target : targets)
                {
                    target->SetType(rule.GetEffect());
                }
            }
            else
            {
                auto func = effects.at(ObjectToProperty(rule.GetEffect()));

                for (auto& target : targets)
                {
                    target->AddProperty(ObjectToProperty(rule.GetEffect()));
                    func(*this, *target, rule);
                }
            }
        }
        else if (rule.GetVerb() == ObjectType::HAS)
        {
            // Not implemented yet
        }
        else if (rule.GetVerb() == ObjectType::MAKE)
        {
            // Not implemented yet
        }
        else
        {
            // throw
        }
    }
}

void Game::determineResult()
{
    auto targets = FindObjectsByProperty(PropertyType::YOU);

    if (targets.empty())
    {
        gameResult_ = GameResult::DEFEAT;
        return;
    }

    for (auto& target : targets)
    {
        auto objs = FindObjectsByPosition(*target);

        for (auto& obj : objs)
        {
            if (obj->HasProperty(PropertyType::WIN))
            {
                gameResult_ = GameResult::WIN;
                return;
            }
        }
    }

    gameResult_ = GameResult::INVALID;
}
}  // namespace Baba
