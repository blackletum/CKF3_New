#ifndef CLIENT_H
#define CLIENT_H

extern void respawn(entvars_t *pev, BOOL fCopyCorpse);
extern BOOL ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
extern void ClientDisconnect(edict_t *pEntity);
extern void ClientKill(edict_t *pEntity);
extern void ClientPutInServer(edict_t *pEntity);
extern void ClientCommand(edict_t *pEntity);
extern void ClientUserInfoChanged(edict_t *pEntity, char *infobuffer);
extern void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
extern void ServerDeactivate(void);
extern void StartFrame(void);
extern void PlayerPostThink(edict_t *pEntity);
extern void PlayerPreThink(edict_t *pEntity);
extern void ParmsNewLevel(void);
extern void ParmsChangeLevel(void);
extern void ClientPrecache(void);
extern const char *GetGameDescription(void);
extern void PlayerCustomization(edict_t *pEntity, customization_t *pCust);
extern void SpectatorConnect(edict_t *pEntity);
extern void SpectatorDisconnect(edict_t *pEntity);
extern void SpectatorThink(edict_t *pEntity);
extern void Sys_Error(const char *error_string);
extern void SetupVisibility(edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas);
extern void UpdateClientData (const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);
extern int AddToFullPack(struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet);
extern void CreateBaseline(int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs);
extern void RegisterEncoders(void);
extern int GetWeaponData(struct edict_s *player, struct weapon_data_s *info);
extern void CmdStart(const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed);
extern void CmdEnd(const edict_t *player);
extern int ConnectionlessPacket(const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size);
extern int GetHullBounds(int hullnumber, float *mins, float *maxs);
extern void CreateInstancedBaselines (void);
extern int InconsistentFile(const edict_t *player, const char *filename, char *disconnect_message);
extern int AllowLagCompensation(void);

extern int HandleMenu_ChooseClass(CBasePlayer* pPlayer, int keys);
extern BOOL HandleMenu_ChooseTeam(CBasePlayer* pPlayer, int keys);

const char* tf_voiceSounds[] =
{
	"demoman_paincrticialdeath01.wav",
	"demoman_paincrticialdeath02.wav",
	"demoman_paincrticialdeath03.wav",
	"demoman_paincrticialdeath04.wav",
	"demoman_paincrticialdeath05.wav",
	"demoman_painsevere01.wav",
	"demoman_painsevere02.wav",
	"demoman_painsevere03.wav",
	"demoman_painsevere04.wav",
	"demoman_painsharp01.wav",
	"demoman_painsharp02.wav",
	"demoman_painsharp03.wav",
	"demoman_painsharp04.wav",
	"demoman_painsharp05.wav",
	"demoman_painsharp06.wav",
	"demoman_painsharp07.wav",
	"demoman_positivevocalization01.wav",
	"demoman_positivevocalization02.wav",
	"demoman_positivevocalization03.wav",
	"demoman_positivevocalization04.wav",
	"demoman_positivevocalization05.wav",
	"engineer_paincrticialdeath01.wav",
	"engineer_paincrticialdeath02.wav",
	"engineer_paincrticialdeath03.wav",
	"engineer_paincrticialdeath04.wav",
	"engineer_paincrticialdeath05.wav",
	"engineer_paincrticialdeath06.wav",
	"engineer_painsevere01.wav",
	"engineer_painsevere02.wav",
	"engineer_painsevere03.wav",
	"engineer_painsevere04.wav",
	"engineer_painsevere05.wav",
	"engineer_painsevere06.wav",
	"engineer_painsevere07.wav",
	"engineer_painsharp01.wav",
	"engineer_painsharp02.wav",
	"engineer_painsharp03.wav",
	"engineer_painsharp04.wav",
	"engineer_painsharp05.wav",
	"engineer_painsharp06.wav",
	"engineer_painsharp07.wav",
	"engineer_painsharp08.wav",
	"engineer_positivevocalization01.wav",
	"heavy_paincrticialdeath01.wav",
	"heavy_paincrticialdeath02.wav",
	"heavy_paincrticialdeath03.wav",
	"heavy_painsevere01.wav",
	"heavy_painsevere02.wav",
	"heavy_painsevere03.wav",
	"heavy_painsharp01.wav",
	"heavy_painsharp02.wav",
	"heavy_painsharp03.wav",
	"heavy_painsharp04.wav",
	"heavy_painsharp05.wav",
	"heavy_positivevocalization01.wav",
	"heavy_positivevocalization02.wav",
	"heavy_positivevocalization03.wav",
	"heavy_positivevocalization04.wav",
	"heavy_positivevocalization05.wav",
	"medic_paincrticialdeath01.wav",
	"medic_paincrticialdeath02.wav",
	"medic_paincrticialdeath03.wav",
	"medic_paincrticialdeath04.wav",
	"medic_painsevere01.wav",
	"medic_painsevere02.wav",
	"medic_painsevere03.wav",
	"medic_painsevere04.wav",
	"medic_painsharp01.wav",
	"medic_painsharp02.wav",
	"medic_painsharp03.wav",
	"medic_painsharp04.wav",
	"medic_painsharp05.wav",
	"medic_painsharp06.wav",
	"medic_painsharp07.wav",
	"medic_painsharp08.wav",
	"medic_positivevocalization01.wav",
	"medic_positivevocalization02.wav",
	"medic_positivevocalization03.wav",
	"medic_positivevocalization05.wav",
	"medic_positivevocalization06.wav",
	"pyro_paincrticialdeath01.wav",
	"pyro_paincrticialdeath02.wav",
	"pyro_paincrticialdeath03.wav",
	"pyro_painsevere01.wav",
	"pyro_painsevere02.wav",
	"pyro_painsevere03.wav",
	"pyro_painsevere04.wav",
	"pyro_painsevere05.wav",
	"pyro_painsevere06.wav",
	"pyro_painsharp01.wav",
	"pyro_painsharp02.wav",
	"pyro_painsharp03.wav",
	"pyro_painsharp04.wav",
	"pyro_painsharp05.wav",
	"pyro_painsharp06.wav",
	"pyro_painsharp07.wav",
	"pyro_painsharp08_special.wav",
	"pyro_positivevocalization01.wav",
	"scout_paincrticialdeath01.wav",
	"scout_paincrticialdeath02.wav",
	"scout_paincrticialdeath03.wav",
	"scout_painsevere01.wav",
	"scout_painsevere02.wav",
	"scout_painsevere03.wav",
	"scout_painsevere04.wav",
	"scout_painsevere05.wav",
	"scout_painsevere06.wav",
	"scout_painsharp01.wav",
	"scout_painsharp02.wav",
	"scout_painsharp03.wav",
	"scout_painsharp04.wav",
	"scout_painsharp05.wav",
	"scout_painsharp06.wav",
	"scout_painsharp07.wav",
	"scout_painsharp08.wav",
	"scout_positivevocalization01.wav",
	"scout_positivevocalization02.wav",
	"scout_positivevocalization03.wav",
	"scout_positivevocalization04.wav",
	"scout_positivevocalization05.wav",
	"sniper_paincrticialdeath01.wav",
	"sniper_paincrticialdeath02.wav",
	"sniper_paincrticialdeath03.wav",
	"sniper_paincrticialdeath04.wav",
	"sniper_painsevere01.wav",
	"sniper_painsevere02.wav",
	"sniper_painsevere03.wav",
	"sniper_painsevere04.wav",
	"sniper_painsharp01.wav",
	"sniper_painsharp02.wav",
	"sniper_painsharp03.wav",
	"sniper_painsharp04.wav",
	"sniper_positivevocalization01.wav",
	"sniper_positivevocalization02.wav",
	"sniper_positivevocalization03.wav",
	"sniper_positivevocalization04.wav",
	"sniper_positivevocalization05.wav",
	"sniper_positivevocalization06.wav",
	"sniper_positivevocalization07.wav",
	"sniper_positivevocalization08.wav",
	"sniper_positivevocalization09.wav",
	"sniper_positivevocalization10.wav",
	"soldier_paincrticialdeath01.wav",
	"soldier_paincrticialdeath02.wav",
	"soldier_paincrticialdeath03.wav",
	"soldier_paincrticialdeath04.wav",
	"soldier_painsevere01.wav",
	"soldier_painsevere02.wav",
	"soldier_painsevere03.wav",
	"soldier_painsevere04.wav",
	"soldier_painsevere05.wav",
	"soldier_painsevere06.wav",
	"soldier_painsharp01.wav",
	"soldier_painsharp02.wav",
	"soldier_painsharp03.wav",
	"soldier_painsharp04.wav",
	"soldier_painsharp05.wav",
	"soldier_painsharp06.wav",
	"soldier_painsharp07.wav",
	"soldier_painsharp08.wav",
	"soldier_positivevocalization01.wav",
	"soldier_positivevocalization02.wav",
	"soldier_positivevocalization03.wav",
	"soldier_positivevocalization04.wav",
	"soldier_positivevocalization05.wav",
	"spy_paincrticialdeath01.wav",
	"spy_paincrticialdeath02.wav",
	"spy_paincrticialdeath03.wav",
	"spy_painsevere01.wav",
	"spy_painsevere02.wav",
	"spy_painsevere03.wav",
	"spy_painsevere04.wav",
	"spy_painsevere05.wav",
	"spy_painsharp01.wav",
	"spy_painsharp02.wav",
	"spy_painsharp03.wav",
	"spy_painsharp04.wav",
	"spy_painsharp05_special.wav",
	"spy_positivevocalization01.wav",
	"spy_positivevocalization02.wav",
	"spy_positivevocalization03.wav",
	"spy_positivevocalization04.wav",
	"spy_positivevocalization05.wav",
	NULL // THIS MUST BE LAST
};

#endif