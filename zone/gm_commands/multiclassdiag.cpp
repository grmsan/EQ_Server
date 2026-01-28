#include "../client.h"
#include "../../common/classes.h"
#include "../../common/rulesys.h"
#include "../../common/strings.h"

static std::string BuildClassList(uint32 bits)
{
	std::string out;
	for (uint8 class_id = 1; class_id <= 16; ++class_id) {
		if ((bits & GetPlayerClassBit(class_id)) == 0) {
			continue;
		}
		if (!out.empty()) {
			out += "/";
		}
		out += GetClassIDName(class_id);
	}
	return out;
}

void command_multiclassdiag(Client *c, const Seperator *sep)
{
	const bool enabled = RuleB(Custom, MulticlassingEnabled);
	const int max_classes = RuleI(Custom, MulticlassMaxClasses);
	const bool is_rof2 = (c->ClientVersion() == EQ::versions::ClientVersion::RoF2);

	const std::string gestalt_raw = c->GetBucket("GestaltClasses");
	const std::string bucket_key = RuleS(Custom, MulticlassBucketKey);
	const std::string configured_raw =
		(!bucket_key.empty() && bucket_key != "GestaltClasses") ? c->GetBucket(bucket_key) : std::string{};

	const uint32 bits = c->GetClassesBits();
	const uint16 mask16 = c->GetClassesBitmask();
	const uint8 classes_count = c->GetClassesCount();

	c->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MCDIAG | name={} char_id={} base_class={} ({}) multiclass_enabled={} max_classes={} classes_bits=0x{:08X} classes_mask16=0x{:04X} classes={}",
			c->GetCleanName(),
			c->CharacterID(),
			static_cast<int>(c->GetClass()),
			GetClassIDName(c->GetClass()),
			enabled ? "true" : "false",
			max_classes,
			static_cast<uint32>(bits),
			static_cast<uint32>(mask16),
			BuildClassList(bits)
		).c_str()
	);

	c->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MCDIAG | client_version={} rof2={} classes_count={} edge_opcode=0x1338",
			static_cast<int>(c->ClientVersion()),
			is_rof2 ? "true" : "false",
			static_cast<int>(classes_count)
		).c_str()
	);

	c->Message(
		Chat::White,
		"%s",
		fmt::format(
			"MCDIAG | bucket GestaltClasses='{}' configured_key='{}' configured_value='{}'",
			gestalt_raw,
			bucket_key,
			configured_raw
		).c_str()
	);

	if (sep && sep->arg[1][0] != '\0' && Strings::ToLower(sep->arg[1]) == "refresh") {
		c->SendEdgeStats();
		c->Message(Chat::White, "MCDIAG | forced SendEdgeStats() (EdgeStatLabel opcode bypass 0x1338)");
	}
}
