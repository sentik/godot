#include "enum.h"
#include "sqlite_orm.h"
#include "scene/main/node.h"

#include <optional>

BETTER_ENUM(EditorAssetFileType, int, Unknown, GenFile, DestFile)


namespace sqlite_orm {

template <>
struct type_printer<String> : public text_printer {};

template <>
struct statement_binder<String> {
	int bind(sqlite3_stmt *stmt, int index, const String &value) const {
		return statement_binder<std::string>().bind(stmt, index, std::string(value.ascii()));
	}
};

template <>
struct field_printer<String> {
	std::string operator()(const String &value) const {
		return std::string(value.ascii());
	}
};

template <>
struct row_extractor<String, void> {
	String extract(const char *row_value) const {
		if (row_value) {
			return row_value;
		} else {
			return {};
		}
	}

	String extract(sqlite3_stmt *stmt, int columnIndex) const {
		if (auto cStr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, columnIndex))) {
			return cStr;
		} else {
			return {};
		}
	}

	String extract(sqlite3_value *value) const {
		if (const auto cStr = reinterpret_cast<const char *>(sqlite3_value_text(value))) {
			return cStr;
		} else {
			return {};
		}
	}
};
} 

#define SQL_ORM_ENUM_BIND(TEnum)                                                                     \
	namespace sqlite_orm {                                                                           \
	template <>                                                                                      \
	struct type_printer<##TEnum## ::_enumerated> : public text_printer {};                           \
	template <>                                                                                      \
	struct statement_binder<##TEnum## ::_enumerated> {	                                             \
		int bind(sqlite3_stmt *stmt, int index, const TEnum## ::_enumerated &value) {                \
			return statement_binder<std::string>().bind(stmt, index, ##TEnum##(value)._to_string()); \
		}                                                                                            \
	};                                                                                               \
	template <>                                                                                      \
	struct field_printer<##TEnum## ::_enumerated> {                                                  \
		std::string operator()(const TEnum## ::_enumerated &value) const {	                         \
			return TEnum##(value)._to_string();					                                     \
		}                                                                                            \
};                                                                                                   \
    template <>                                                                                      \
	struct row_extractor<TEnum## ::_enumerated> {	                                                 \
		TEnum##::_enumerated extract(const char *row_value) {	                                     \
				return TEnum## ::_from_string_nocase(row_value);                                     \
		}	                                                                                         \
		TEnum##::_enumerated extract(sqlite3_stmt *stmt, int columnIndex) {	                         \
			auto str = sqlite3_column_text(stmt, columnIndex);		                                 \
			return this->extract((const char *)str);	                                             \
		}	                                                                                         \
};		                                                                                             \
}

SQL_ORM_ENUM_BIND(EditorAssetFileType)

struct EditorAsset {
	int32_t asset_id = 0;

	//	remap
	String file_path;
	String resource_name;
	String source_md5;
	String importer_name;
	String group_file;
	String resource_type;
	ResourceUID::ID uid = ResourceUID::INVALID_ID;
	int32_t importer_version;
	String generator_parameters;

	time_t modified_time = 0;
	time_t import_modified_time = 0;
	bool is_valid = false;

	//	deps
	String source_file;

	//	params
	String metadata;
	bool has_value() const {
		return asset_id != 0;
	}

	//EditorAsset(const String &file_path) :
	//		file_path(file_path) {
	//	
	//}

};

struct EditorAssetParam {
	int32_t param_id = 0;
	decltype(EditorAsset::asset_id) asset_id = 0;
	String key;
	String value;
};



struct EditorAssetFile {
	int32_t file_id = 0;
	decltype(EditorAsset::asset_id) asset_id = 0;
	String variant_name;
	String path;
	String hash;
	EditorAssetFileType::_enumerated file_type = EditorAssetFileType::Unknown;
	std::vector<char> blob;
};


	static auto get_editor_asset_table() {
	return sqlite_orm::make_table("editor_assets",
			sqlite_orm::make_column("asset_id", &EditorAsset::asset_id, sqlite_orm::primary_key(), sqlite_orm::autoincrement()),
			sqlite_orm::make_column("file_path", &EditorAsset::file_path, sqlite_orm::unique()),
			sqlite_orm::make_column("uid", &EditorAsset::uid, sqlite_orm::unique()),
			sqlite_orm::make_column("resource_name", &EditorAsset::resource_name),
			sqlite_orm::make_column("source_md5", &EditorAsset::source_md5),
			sqlite_orm::make_column("resource_type", &EditorAsset::resource_type),
			sqlite_orm::make_column("importer_name", &EditorAsset::importer_name),
			sqlite_orm::make_column("importer_version", &EditorAsset::importer_version),
			sqlite_orm::make_column("source_file", &EditorAsset::source_file),
			sqlite_orm::make_column("group_file", &EditorAsset::group_file),
			sqlite_orm::make_column("modified_time", &EditorAsset::modified_time),
			sqlite_orm::make_column("metadata", &EditorAsset::metadata),
			sqlite_orm::make_column("import_modified_time", &EditorAsset::import_modified_time));
}

static auto get_editor_asset_param_table() {
	return sqlite_orm::make_table("editor_assets_params",
			sqlite_orm::make_column("param_id", &EditorAssetParam::param_id, sqlite_orm::primary_key(), sqlite_orm::autoincrement()),
			sqlite_orm::make_column("asset_id", &EditorAssetParam::asset_id),
			sqlite_orm::make_column("key", &EditorAssetParam::key),
			sqlite_orm::make_column("value", &EditorAssetParam::value),
			sqlite_orm::foreign_key(&EditorAssetParam::asset_id).references(&EditorAsset::asset_id));
}
static auto get_editor_asset_files_table() {
	return sqlite_orm::make_table("editor_assets_files",
			sqlite_orm::make_column("file_id", &EditorAssetFile::file_id, sqlite_orm::primary_key(), sqlite_orm::autoincrement()),
			sqlite_orm::make_column("asset_id", &EditorAssetFile::asset_id),
			sqlite_orm::make_column("variant_name", &EditorAssetFile::variant_name),
			sqlite_orm::make_column("path", &EditorAssetFile::path),
			sqlite_orm::make_column("hash", &EditorAssetFile::hash),
			sqlite_orm::make_column("file_type", &EditorAssetFile::file_type),
			sqlite_orm::make_column("blob", &EditorAssetFile::blob),
			sqlite_orm::foreign_key(&EditorAssetFile::asset_id).references(&EditorAsset::asset_id));
}

static auto create_storage(const std::string &filePath) {
	return sqlite_orm::make_storage(
			filePath,
			get_editor_asset_table(),
			get_editor_asset_param_table(),
			get_editor_asset_files_table());
}

class EditorFileSystemDb : public Node {
	private:
	static EditorFileSystemDb *singleton;

	public:
	static EditorFileSystemDb* get_singleton() {
		return singleton;
	}

public:
	using Storage = std::result_of<decltype (&create_storage)(const std::string &)>::type;
	std::unique_ptr<Storage> storage;

	EditorFileSystemDb();

	bool asset_exist(const String &p_path) const;
	std::optional<EditorAsset> asset_get(const String &p_path) const;
	std::vector<EditorAssetParam> asset_params_get(const int32_t &asset_id) const;
	std::vector<EditorAssetFile> asset_files_get(const int32_t &asset_id, const EditorAssetFileType::_enumerated &file_type = EditorAssetFileType::Unknown) const;

	void asset_files_replace(const int32_t &asset_id, std::vector<EditorAssetFile>& files) const;
	void asset_params_replace(const int32_t &asset_id, std::vector<EditorAssetParam> &params) const;

	void asset_add_or_update(EditorAsset &asset) const;
	static void create_singleton();
};
