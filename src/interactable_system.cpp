#include "interactable_system.h"
#include <algorithm>

const std::unordered_map<std::string, InteractionTag>& CommandVerbRegistry::get_mappings() {
    static const std::unordered_map<std::string, InteractionTag> mappings = {
        {"chop", InteractionTag::Choppable},
        {"cut", InteractionTag::Choppable},
        {"cleave", InteractionTag::Choppable},
        {"hew", InteractionTag::Choppable},
        {"fell", InteractionTag::Choppable},
        
        {"burn", InteractionTag::Burnable},
        {"ignite", InteractionTag::Burnable},
        {"scorch", InteractionTag::Burnable},
        {"light", InteractionTag::Burnable},
        {"fire", InteractionTag::Burnable},
        {"torch", InteractionTag::Burnable},
        
        {"forage", InteractionTag::Forageable},
        {"gather", InteractionTag::Forageable},
        {"harvest", InteractionTag::Forageable},
        {"pick", InteractionTag::Forageable},

        {"scrape", InteractionTag::Scrapeable},
        {"peel", InteractionTag::Scrapeable},
        {"shave", InteractionTag::Scrapeable},
        {"grate", InteractionTag::Scrapeable},

        {"clear", InteractionTag::Clearable},
        {"clean", InteractionTag::Clearable},
        {"sweep", InteractionTag::Clearable},
        {"remove", InteractionTag::Clearable},
        {"tidy", InteractionTag::Clearable},
        
        {"search", InteractionTag::Searchable},
        {"examine", InteractionTag::Searchable},
        {"inspect", InteractionTag::Searchable},
        {"look", InteractionTag::Searchable},
        {"scavenge", InteractionTag::Searchable},
        {"rummage", InteractionTag::Searchable}
    };
    return mappings;
}

std::string AdvancedInteractableTemplate::get_description(const std::unordered_map<std::string, InteractionTag>& runtime_tag_states) const {
    std::string desc = main_description;
    for (const auto& pair : features) {
        const auto& feature = pair.second;
        auto it = runtime_tag_states.find(feature.keyword);
        if (it != runtime_tag_states.end()) {
            InteractionTag current_tag = it->second;
            // If tag has mutated to the success mutation tag, do replacement
            if (current_tag == feature.success_mutation_tag && current_tag != feature.active_tag) {
                if (!feature.desc_substring_to_replace.empty() && !feature.mutated_desc_substring.empty()) {
                    size_t pos = desc.find(feature.desc_substring_to_replace);
                    if (pos != std::string::npos) {
                        desc.replace(pos, feature.desc_substring_to_replace.length(), feature.mutated_desc_substring);
                    }
                }
            }
        }
    }
    return desc;
}

void ObjectDictionaryManager::initialize_dictionaries() {
    master_registry.clear();
    biome_pools.clear();

    // 1. game_trail
    {
        AdvancedInteractableTemplate t;
        t.id = "game_trail";
        t.name = "game trail";
        t.main_description = "A faint path worn down by forest animals. Prints of small game are visible in the soft ground.";
        
        SubElementFeature f_tracks;
        f_tracks.keyword = "tracks";
        f_tracks.active_tag = InteractionTag::Searchable;
        f_tracks.success_msg = "You inspect the tracks and deduce they belong to a herd of deer.";
        f_tracks.failure_msg = "You cannot do that to the tracks.";
        t.features[f_tracks.keyword] = f_tracks;

        SubElementFeature f_herbs;
        f_herbs.keyword = "herbs";
        f_herbs.active_tag = InteractionTag::Forageable;
        f_herbs.success_msg = "You gather some useful wild herbs from the edge of the trail.";
        f_herbs.failure_msg = "You cannot do that to the herbs.";
        f_herbs.success_mutation_tag = InteractionTag::None;
        t.features[f_herbs.keyword] = f_herbs;

        master_registry[t.id] = t;
    }

    // 2. delicate_berries
    {
        AdvancedInteractableTemplate t;
        t.id = "delicate_berries";
        t.name = "delicate berries";
        t.main_description = "A dense bush heavy with small, bright red berries that give off a sweet aroma.";
        
        SubElementFeature f_berries;
        f_berries.keyword = "berries";
        f_berries.active_tag = InteractionTag::Forageable;
        f_berries.success_msg = "You carefully pick the ripe red berries.";
        f_berries.failure_msg = "You cannot do that to the berries.";
        f_berries.success_mutation_tag = InteractionTag::None;
        f_berries.desc_substring_to_replace = "heavy with small, bright red berries that give off a sweet aroma";
        f_berries.mutated_desc_substring = "stripped bare of its fruit";
        t.features[f_berries.keyword] = f_berries;

        SubElementFeature f_bush;
        f_bush.keyword = "bush";
        f_bush.active_tag = InteractionTag::Burnable;
        f_bush.valid_tools = {"torch", "flint"};
        f_bush.success_msg = "You set the berry bush ablaze, reducing it to ashes.";
        f_bush.failure_msg = "You cannot do that to the bush.";
        f_bush.no_tool_msg = "You need a fire source to burn this bush.";
        f_bush.success_mutation_tag = InteractionTag::None;
        f_bush.desc_substring_to_replace = "A dense bush heavy with small, bright red berries that give off a sweet aroma.";
        f_bush.mutated_desc_substring = "A scorched patch of ash where a berry bush once stood.";
        t.features[f_bush.keyword] = f_bush;

        master_registry[t.id] = t;
    }

    // 3. briar_patch
    {
        AdvancedInteractableTemplate t;
        t.id = "briar_patch";
        t.name = "briar patch";
        t.main_description = "A tangled, thorny mound of briars. It blocks navigation and has sharp, defensive thorns.";
        
        SubElementFeature f_thorns;
        f_thorns.keyword = "thorns";
        f_thorns.active_tag = InteractionTag::Choppable;
        f_thorns.valid_tools = {"axe", "cleaver", "machete"};
        f_thorns.success_msg = "You clear away the thorny brambles.";
        f_thorns.failure_msg = "You cannot do that to the thorns.";
        f_thorns.no_tool_msg = "You need a cutting tool to chop these thorns.";
        f_thorns.success_mutation_tag = InteractionTag::None;
        f_thorns.desc_substring_to_replace = "It blocks navigation and has sharp, defensive thorns.";
        f_thorns.mutated_desc_substring = "The thorns have been chopped away, leaving a clear path.";
        t.features[f_thorns.keyword] = f_thorns;

        SubElementFeature f_briars;
        f_briars.keyword = "briars";
        f_briars.active_tag = InteractionTag::Burnable;
        f_briars.valid_tools = {"torch", "flint"};
        f_briars.success_msg = "The briar patch burns quickly, leaving cleared ground.";
        f_briars.failure_msg = "You cannot do that to the briars.";
        f_briars.no_tool_msg = "You need a fire source to burn the briars.";
        f_briars.success_mutation_tag = InteractionTag::None;
        f_briars.desc_substring_to_replace = "A tangled, thorny mound of briars.";
        f_briars.mutated_desc_substring = "A smoldering heap of charcoal.";
        t.features[f_briars.keyword] = f_briars;

        SubElementFeature f_nest;
        f_nest.keyword = "nest";
        f_nest.active_tag = InteractionTag::Searchable;
        f_nest.success_msg = "You search the hidden spaces within the briars and find a small animal nest.";
        f_nest.failure_msg = "You cannot do that to the nest.";
        f_nest.success_mutation_tag = InteractionTag::None;
        t.features[f_nest.keyword] = f_nest;

        master_registry[t.id] = t;
    }

    // 4. dry_brush
    {
        AdvancedInteractableTemplate t;
        t.id = "dry_brush";
        t.name = "dry brush";
        t.main_description = "A pile of brittle, dried wood and dead grass, dry as tinder.";
        
        SubElementFeature f_brush;
        f_brush.keyword = "brush";
        f_brush.active_tag = InteractionTag::Burnable;
        f_brush.valid_tools = {"torch", "flint"};
        f_brush.success_msg = "The dry brush ignites instantly, crackling fiercely.";
        f_brush.failure_msg = "You cannot do that to the brush.";
        f_brush.no_tool_msg = "You need a fire source to ignite the brush.";
        f_brush.success_mutation_tag = InteractionTag::None;
        f_brush.desc_substring_to_replace = "brittle, dried wood and dead grass, dry as tinder";
        f_brush.mutated_desc_substring = "scorched soot and ashes";
        t.features[f_brush.keyword] = f_brush;

        SubElementFeature f_ground;
        f_ground.keyword = "ground";
        f_ground.active_tag = InteractionTag::Searchable;
        f_ground.success_msg = "You search the dry ground and find nothing but dry soil.";
        f_ground.failure_msg = "You cannot do that.";
        t.features[f_ground.keyword] = f_ground;

        master_registry[t.id] = t;
    }

    // 5. fallen_log
    {
        AdvancedInteractableTemplate t;
        t.id = "fallen_log";
        t.name = "fallen log";
        t.main_description = "A decaying tree trunk lying across the dirt, covered in green moss and sticky sap.";
        
        SubElementFeature f_sap;
        f_sap.keyword = "sap";
        f_sap.active_tag = InteractionTag::Scrapeable;
        f_sap.valid_tools = {"cleaver", "knife", "dagger"};
        f_sap.success_msg = "You scrape the sticky sap off the log.";
        f_sap.failure_msg = "You cannot do that to the sap.";
        f_sap.no_tool_msg = "You need a blade to scrape the sap.";
        f_sap.success_mutation_tag = InteractionTag::None;
        f_sap.desc_substring_to_replace = "and sticky sap";
        f_sap.mutated_desc_substring = "";
        t.features[f_sap.keyword] = f_sap;

        SubElementFeature f_trunk;
        f_trunk.keyword = "trunk";
        f_trunk.active_tag = InteractionTag::Choppable;
        f_trunk.valid_tools = {"axe", "cleaver"};
        f_trunk.success_msg = "You chop the decaying log into firewood.";
        f_trunk.failure_msg = "You cannot do that to the trunk.";
        f_trunk.no_tool_msg = "You need an axe or cleaver to chop the trunk.";
        f_trunk.success_mutation_tag = InteractionTag::None;
        f_trunk.desc_substring_to_replace = "A decaying tree trunk lying across the dirt";
        f_trunk.mutated_desc_substring = "A pile of chopped wood chips";
        t.features[f_trunk.keyword] = f_trunk;

        SubElementFeature f_moss;
        f_moss.keyword = "moss";
        f_moss.active_tag = InteractionTag::Searchable;
        f_moss.success_msg = "You inspect the moss and find tiny insects underneath.";
        f_moss.failure_msg = "You cannot do that to the moss.";
        t.features[f_moss.keyword] = f_moss;

        master_registry[t.id] = t;
    }

    // 6. ancient_obelisk
    {
        AdvancedInteractableTemplate t;
        t.id = "ancient_obelisk";
        t.name = "ancient obelisk";
        t.main_description = "A tall stone pillar carved with alien glyphs and covered in creeping vines.";
        
        SubElementFeature f_glyphs;
        f_glyphs.keyword = "glyphs";
        f_glyphs.active_tag = InteractionTag::Searchable;
        f_glyphs.success_msg = "You inspect the glowing glyphs and feel a pulse of magic.";
        f_glyphs.failure_msg = "You cannot do that to the glyphs.";
        t.features[f_glyphs.keyword] = f_glyphs;

        SubElementFeature f_vines;
        f_vines.keyword = "vines";
        f_vines.active_tag = InteractionTag::Clearable;
        f_vines.valid_tools = {"axe", "cleaver", "knife", "machete"};
        f_vines.success_msg = "You clear the creeping vines covering the stone.";
        f_vines.failure_msg = "You cannot do that to the vines.";
        f_vines.no_tool_msg = "You need a blade to clear the vines.";
        f_vines.success_mutation_tag = InteractionTag::None;
        f_vines.desc_substring_to_replace = "and covered in creeping vines";
        f_vines.mutated_desc_substring = "";
        t.features[f_vines.keyword] = f_vines;

        master_registry[t.id] = t;
    }

    // 7. driftwood
    {
        AdvancedInteractableTemplate t;
        t.id = "driftwood";
        t.name = "driftwood pile";
        t.main_description = "A heap of twisted, sun-bleached driftwood wood washed ashore on the sand.";
        
        SubElementFeature f_wood;
        f_wood.keyword = "wood";
        f_wood.active_tag = InteractionTag::Choppable;
        f_wood.valid_tools = {"axe"};
        f_wood.success_msg = "You split the tough driftwood into smaller pieces.";
        f_wood.failure_msg = "You cannot do that to the wood.";
        f_wood.no_tool_msg = "You need an axe to chop this wood.";
        f_wood.success_mutation_tag = InteractionTag::None;
        f_wood.desc_substring_to_replace = "A heap of twisted, sun-bleached driftwood wood";
        f_wood.mutated_desc_substring = "Splintered wood remnants";
        t.features[f_wood.keyword] = f_wood;

        SubElementFeature f_pile;
        f_pile.keyword = "pile";
        f_pile.active_tag = InteractionTag::Burnable;
        f_pile.valid_tools = {"torch", "flint"};
        f_pile.success_msg = "The driftwood pile burns with a salty, colorful flame.";
        f_pile.failure_msg = "You cannot do that to the pile.";
        f_pile.no_tool_msg = "You need a fire source to burn the pile.";
        f_pile.success_mutation_tag = InteractionTag::None;
        f_pile.desc_substring_to_replace = "A heap of twisted, sun-bleached driftwood wood washed ashore on the sand.";
        f_pile.mutated_desc_substring = "A mound of salty grey ash.";
        t.features[f_pile.keyword] = f_pile;

        SubElementFeature f_sand;
        f_sand.keyword = "sand";
        f_sand.active_tag = InteractionTag::Searchable;
        f_sand.success_msg = "You sift through the sand around the driftwood and find some smooth sea glass.";
        f_sand.failure_msg = "You cannot do that to the sand.";
        t.features[f_sand.keyword] = f_sand;

        master_registry[t.id] = t;
    }

    // 8. frozen_shrub
    {
        AdvancedInteractableTemplate t;
        t.id = "frozen_shrub";
        t.name = "frozen shrub";
        t.main_description = "A low bush completely encased in sparkling ice, hiding its green leaves.";
        
        SubElementFeature f_ice;
        f_ice.keyword = "ice";
        f_ice.active_tag = InteractionTag::Clearable;
        f_ice.valid_tools = {"axe", "cleaver", "pickaxe"};
        f_ice.success_msg = "You clear away the thick crust of ice.";
        f_ice.failure_msg = "You cannot do that to the ice.";
        f_ice.no_tool_msg = "You need a tool to chip away the ice.";
        f_ice.success_mutation_tag = InteractionTag::None;
        f_ice.desc_substring_to_replace = "completely encased in sparkling ice";
        f_ice.mutated_desc_substring = "dripping wet";
        t.features[f_ice.keyword] = f_ice;

        SubElementFeature f_leaves;
        f_leaves.keyword = "leaves";
        f_leaves.active_tag = InteractionTag::Forageable;
        f_leaves.success_msg = "You forage the frozen, medicinal leaves from the shrub.";
        f_leaves.failure_msg = "You cannot do that to the leaves.";
        f_leaves.success_mutation_tag = InteractionTag::None;
        t.features[f_leaves.keyword] = f_leaves;

        master_registry[t.id] = t;
    }

    // Define Biome pools (mappings correspond to step_assign_biomes)
    biome_pools[1] = {"driftwood", "delicate_berries"};
    biome_pools[2] = {"frozen_shrub", "ancient_obelisk"};
    biome_pools[3] = {"frozen_shrub", "ancient_obelisk"};
    biome_pools[4] = {"game_trail", "delicate_berries", "briar_patch", "fallen_log", "ancient_obelisk"};
    biome_pools[5] = {"dry_brush", "ancient_obelisk"};
    biome_pools[6] = {"game_trail", "delicate_berries", "dry_brush", "ancient_obelisk"};
    biome_pools[7] = {"game_trail", "delicate_berries", "briar_patch", "fallen_log", "ancient_obelisk"};
    biome_pools[8] = {"game_trail", "delicate_berries", "briar_patch", "fallen_log", "ancient_obelisk"};
    biome_pools[9] = {"dry_brush", "ancient_obelisk"};
    biome_pools[10] = {"game_trail", "delicate_berries", "dry_brush", "ancient_obelisk"};
    biome_pools[11] = {"game_trail", "delicate_berries", "briar_patch", "fallen_log", "ancient_obelisk"};
    biome_pools[12] = {"game_trail", "delicate_berries", "briar_patch", "fallen_log", "ancient_obelisk"};
    biome_pools[15] = {"driftwood", "delicate_berries"};
}

const AdvancedInteractableTemplate* ObjectDictionaryManager::get_template(const std::string& template_id) const {
    auto it = master_registry.find(template_id);
    if (it != master_registry.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<std::string> ObjectDictionaryManager::get_pool_for_biome(int biome_id) const {
    auto it = biome_pools.find(biome_id);
    if (it != biome_pools.end()) {
        return it->second;
    }
    return {};
}
