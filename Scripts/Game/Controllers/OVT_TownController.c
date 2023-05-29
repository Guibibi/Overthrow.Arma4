class OVT_TownControllerComponentClass: OVT_ComponentClass
{
};

class OVT_TownControllerComponent: OVT_Component
{
	protected OVT_TownManagerComponent m_TownManager;
	protected OVT_EconomyManagerComponent m_Economy;
	protected OVT_TownData m_Town;
	protected EntityID m_GunDealerID;
	
	protected bool m_bCiviliansSpawned;
	
	protected ref array<ref EntityID> m_aCivilians;
	
	override void OnPostInit(IEntity owner)
	{	
		super.OnPostInit(owner);
		
		if (SCR_Global.IsEditMode()) return;
		
		m_TownManager = OVT_Global.GetTowns();
		m_Economy = OVT_Global.GetEconomy();
		m_Town = m_TownManager.GetNearestTown(GetOwner().GetOrigin());
		m_aCivilians = new array<ref EntityID>;
		
		if(!Replication.IsServer()) return;
		
		if(m_Town.size > 1)
			GetGame().GetCallqueue().CallLater(SpawnGunDealer, 0);
		
		GetGame().GetCallqueue().CallLater(CheckSpawnCivilian, 10000, true);
		
		CheckSpawnCivilian();
	}
	
	protected void CheckSpawnCivilian()
	{
		if(m_bCiviliansSpawned)
		{
			if(!OVT_Global.PlayerInRange(m_Town.location, m_Config.m_iCivilianSpawnDistance)) {
				DespawnCivilians();
			}
		}else{
			if(!OVT_Global.PlayerInRange(m_Town.location, m_Config.m_iCivilianSpawnDistance)) return;
			SpawnCivilians();
		}
	}
	
	protected void DespawnCivilians()
	{
		m_bCiviliansSpawned = false;
		foreach(EntityID id : m_aCivilians)
		{
			SCR_AIGroup aigroup = GetGroup(id);			
			SCR_EntityHelper.DeleteEntityAndChildren(aigroup);
		}
		
		m_aCivilians.Clear();
	}
	
	protected SCR_AIGroup GetGroup(EntityID id)
	{
		IEntity ent = GetGame().GetWorld().FindEntityByID(id);
		return SCR_AIGroup.Cast(ent);
	}
	
	protected void SpawnCivilians()
	{
		if(m_bCiviliansSpawned) return;
		m_bCiviliansSpawned = true;
		int numciv = Math.Round((float)m_Town.population * m_Config.m_fCivilianSpawnRate);
		
		for(int i=0; i<numciv; i++)
		{
			SpawnCivilian();
		}
	}
	
	protected void SpawnCivilian()
	{
		vector spawnPosition = s_AIRandomGenerator.GenerateRandomPointInRadius(0,m_TownManager.GetTownRange(m_Town),m_Town.location,false);
		spawnPosition[1] = 1;
		
		IEntity target = m_TownManager.GetRandomHouseInTown(m_Town);
		
		BaseWorld world = GetGame().GetWorld();
		
		spawnPosition = OVT_Global.FindSafeSpawnPosition(spawnPosition);
		IEntity civ = OVT_Global.SpawnEntityPrefab(m_Config.m_pCivilianPrefab, spawnPosition);
		
		EntityID civId = civ.GetID();
		
		m_aCivilians.Insert(civId);
		
		SCR_AIGroup aigroup = GetGroup(civId);
		
		array<AIWaypoint> queueOfWaypoints = new array<AIWaypoint>();
		
		queueOfWaypoints.Insert(m_Config.SpawnPatrolWaypoint(target.GetOrigin()));			
		queueOfWaypoints.Insert(m_Config.SpawnWaitWaypoint(target.GetOrigin(), s_AIRandomGenerator.RandFloatXY(15, 50)));
		
		queueOfWaypoints.Insert(m_Config.SpawnPatrolWaypoint(spawnPosition));			
		queueOfWaypoints.Insert(m_Config.SpawnWaitWaypoint(spawnPosition, s_AIRandomGenerator.RandFloatXY(15, 50)));
		
		AIWaypointCycle cycle = AIWaypointCycle.Cast(m_Config.SpawnWaypoint(m_Config.m_pCycleWaypointPrefab, spawnPosition));
		cycle.SetWaypoints(queueOfWaypoints);
		cycle.SetRerunCounter(-1);
		aigroup.AddWaypoint(cycle);
	}
	
	protected void SpawnGunDealer()
	{
		vector spawnPosition;
		
		if(m_Town.gunDealerPosition && m_Town.gunDealerPosition[0] != 0)
		{
			spawnPosition = m_Town.gunDealerPosition;
		}else{
			IEntity house = m_TownManager.GetRandomHouseInTown(m_Town);		
			spawnPosition = house.GetOrigin();
		}
		
		BaseWorld world = GetGame().GetWorld();
		
		spawnPosition = OVT_Global.FindSafeSpawnPosition(spawnPosition);
		
		IEntity dealer = OVT_Global.SpawnEntityPrefab(m_Config.m_pGunDealerPrefab, spawnPosition);
		
		m_GunDealerID = dealer.GetID();
		
		m_Town.gunDealerPosition = spawnPosition;
		
		int townID = m_TownManager.GetTownID(m_Town);
		
		OVT_ShopComponent shop = OVT_ShopComponent.Cast(dealer.FindComponent(OVT_ShopComponent));
		shop.m_iTownId = townID;
		
		foreach(OVT_ShopInventoryItem item : m_Economy.m_aGunDealerItems)
		{
			array<SCR_EntityCatalogEntry> entries();
			m_Economy.FindInventoryItems(item.m_eItemType, item.m_eItemMode, item.m_sFind, entries);
			
			foreach(SCR_EntityCatalogEntry entry : entries)
			{
				int id = m_Economy.GetInventoryId(entry.GetPrefab());											
				int num = Math.Round(s_AIRandomGenerator.RandFloatXY(1,m_Economy.GetTownMaxStock(townID, id)));
				
				shop.AddToInventory(id, num);
			}
		}
		
		GetGame().GetCallqueue().CallLater(SetDealerFaction, 500);
		
		m_Economy.RegisterGunDealer(m_GunDealerID);
	}
	
	protected void SetDealerFaction()
	{
		IEntity dealer = GetGame().GetWorld().FindEntityByID(m_GunDealerID);
		FactionAffiliationComponent faction = FactionAffiliationComponent.Cast(dealer.FindComponent(FactionAffiliationComponent));
		if(!faction){
			Print("Gun dealer spawn prefab is missing FactionAffiliationComponent!");
		}else{			
			faction.SetAffiliatedFactionByKey("");
		}
	}
}