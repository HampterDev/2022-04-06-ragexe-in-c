#ifndef SKILLS_SKILL_TREE_H
#define SKILLS_SKILL_TREE_H

struct Tree;
struct SkillInfo;
typedef unsigned char (*SkillInfoVisitor)(void *context, const struct SkillInfo *skill);

void skill_tree_visit(const struct Tree *tabs, SkillInfoVisitor visit, void *context);

#endif
