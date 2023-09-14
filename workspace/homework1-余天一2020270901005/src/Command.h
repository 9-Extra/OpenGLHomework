#pragma once
#include <deque>
#include <vector>
#include <memory>
#include <string>
#include <sstream>
//任务控制系统

class Command{
public:
    virtual void invoke() = 0;//执行
    virtual void revoke() = 0;//撤销
    virtual std::string decription() {return "没有描述的命令";};

    virtual ~Command() = default;
};

class GroupCommand: public Command{
public:
    GroupCommand() {}

    template<class ...Args>
    GroupCommand(Args&&... args){
        emplace_command(std::forward<Args>(args)...);
    }

    template<class T, class ...Args>
    void emplace_command(T&& command, Args&&... args){
        sub_commands.emplace_back(std::forward<T>(command));
        if constexpr (sizeof...(args) != 0){
            emplace_command(std::forward<Args>(args)...);
        }
    }

    virtual void invoke() override{
        for(auto& c : sub_commands){
            c->invoke();
        }
    }
    virtual void revoke() override{
        //按相反的顺序撤销
        for (auto it = sub_commands.rbegin(); it != sub_commands.rend(); ++it) {
            (*it)->revoke();
        }
    }

    virtual std::string decription() override{
        std::stringstream format;
        format << "组: {\n";
        for(auto& c : sub_commands){
            format << "\t" << c->decription() << "\n";
        }
        format << "}\n";
        return format.str();
    }
private:
    std::vector<std::unique_ptr<Command>> sub_commands;//按执行顺序记录
};

class CommandContainer final{
public:
    CommandContainer(){}
    //撤销最后一步操作
    void revoke_last_command(){
        command_stack.back()->revoke();
        command_stack.pop_back();
    }

    void push_command(std::unique_ptr<Command>&& command){
        if (command_stack.size() == CommandStackMaxSize){
            command_stack.pop_front();
        }
        command_stack.push_back(std::move(command));
    }


private:
    const static size_t CommandStackMaxSize = 200;

    std::deque<std::unique_ptr<Command>> command_stack;
};