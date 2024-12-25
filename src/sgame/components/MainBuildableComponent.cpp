#include "common/Common.h"
#include "MainBuildableComponent.h"
#include "shared/bg_mod.h"

MainBuildableComponent::MainBuildableComponent(Entity& entity, BuildableComponent& r_BuildableComponent)
	: MainBuildableComponentBase(entity, r_BuildableComponent), lastAttackWarnLevel(-1)
{}

void MainBuildableComponent::HandleDamage(float amount, gentity_t* source, Util::optional<glm::vec3> /*location*/,
                                          Util::optional<glm::vec3> /*direction*/, int /*flags*/, meansOfDeath_t meansOfDeath) {
	// Warn the team about an attack.
	if (G_IsWarnableMOD(meansOfDeath)) {
		float healthFraction = GetBuildableComponent().GetHealthComponent().HealthFraction();

		int warnLevel = 0;
		if (healthFraction < 0.75f) warnLevel++;
		if (healthFraction < 0.50f) warnLevel++;
		if (healthFraction < 0.25f) warnLevel++;

		if (warnLevel && (warnLevel > lastAttackWarnLevel || level.time > nextAttackWarning)) {
			nextAttackWarning   = level.time + ATTACKWARN_PRIMARY_PERIOD;
			lastAttackWarnLevel = warnLevel;

			// TODO: Use TeamComponent/LocationComponent.
			G_BroadcastEvent(EV_MAIN_UNDER_ATTACK, warnLevel, entity.oldEnt->buildableTeam);
			Beacon::NewArea(BCT_DEFEND, VEC2GLM(entity.oldEnt->s.origin), entity.oldEnt->buildableTeam);
		}

		// Warn the team and admins if somebody is trying to TK the main buildable.
		if (source && source->client && !( source->r.svFlags & SVF_BOT ) // bots won't trigger the warning
		    && G_Team(source) == entity.oldEnt->buildableTeam
		    && level.team[entity.oldEnt->buildableTeam].lastMainBuildableTKWarn + 1000 < level.time) {

			level.team[entity.oldEnt->buildableTeam].lastMainBuildableTKWarn = level.time;
			G_TeamCommand(entity.oldEnt->buildableTeam,
			              va("print_tr %s %s %s", QQ(N_("^7$1$ ^3DAMAGED ^7by ^1TEAMMATE ^7$2$")),
			                                      BG_Buildable(entity.oldEnt->s.modelindex)->humanName,
			                                      Quote(source->client->pers.netname)));
			G_TeamCommand(entity.oldEnt->buildableTeam,
			              va("cp_tr %s %s %s", QQ(N_("^7$1$ ^3DAMAGED ^7by ^1TEAMMATE ^7$2$")),
			                                      BG_Buildable(entity.oldEnt->s.modelindex)->humanName,
			                                      Quote(source->client->pers.netname)));
			/*G_AdminMessage(nullptr, va("^7%s ^3DAMAGED ^7by ^1TEAMMATE ^7[^5#%d^7] %s",
			                           BG_Buildable(entity.oldEnt->s.modelindex)->humanName,
			                           source->client->num(),
			                           source->client->pers.netname));*/

			G_LogPrintf("TeamDmg: %d %d %s %.0f: %s bled for %.0fhp by teammate %s",
			            source->client->num(),
			            entity.oldEnt->num(),
			            bg_meansOfDeathData[meansOfDeath].name,
			            amount,
			            BG_Buildable(entity.oldEnt->s.modelindex)->humanName,
			            amount,
			            source->client->pers.netname);

			std::string msg = Quote(va("^7%s ^3DAMAGED ^7by ^1TEAMMATE ^7[^5#%d^7] %s",
			                           BG_Buildable(entity.oldEnt->s.modelindex)->humanName,
			                           source->client->num(),
			                           source->client->pers.netname));

			for (int i = 0; i < level.maxclients; i++)
			{
				gentity_t* admin = &g_entities[ i ];
				if (G_admin_permission(admin, ADMF_ADMINCHAT) && G_admin_permission(admin, ADMF_SPEC_ALLCHAT)
				    && G_Team(admin) != entity.oldEnt->buildableTeam
				    && G_Team(admin) != TEAM_NONE)
				{
					trap_SendServerCommand(i, va("chat %d %d %s", -1, SAY_ADMINS, msg.c_str()));
				}
			}
		}
	}
}

void MainBuildableComponent::HandleDie(gentity_t* /*killer*/, meansOfDeath_t meansOfDeath) {
	G_UpdateBuildPointBudgets();

	if (G_IsWarnableMOD(meansOfDeath)) {
		// TODO: Use TeamComponent.
		G_BroadcastEvent(EV_MAIN_DYING, 0, entity.oldEnt->buildableTeam);
	}
}

void MainBuildableComponent::HandleFinishConstruction() {
	G_UpdateBuildPointBudgets();

	// TODO: Generate event that informs team here.
}
