#include "editor_file_system_db.h"
#pragma optimize("", off)


void errorLogCallback(void *pStatement, int iErrCode, const char *zMsg) {
	if (pStatement) {
		fprintf(stderr, "%s\n(%d) %s\n", *(char **)pStatement, iErrCode, zMsg);

	} else {
		fprintf(stderr, "(%d) %s\n", iErrCode, zMsg);
	}
}
		  
EditorFileSystemDb::EditorFileSystemDb() {
	WARN_PRINT("storage->libversion: " + String(storage->libversion().c_str()));

	const auto thread_safe_mode = sqlite_orm::threadsafe();
	if (thread_safe_mode == 0)
		WARN_PRINT("sqlite_orm::threadsafe: Single-thread | " + itos(thread_safe_mode));
	else if (thread_safe_mode == 1)
		WARN_PRINT("sqlite_orm::threadsafe: Multi-thread | " + itos(thread_safe_mode));

	char **pStatement = NULL;
	sqlite3_config(SQLITE_CONFIG_LOG, errorLogCallback, pStatement);

	const auto configure_result = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
	if (configure_result != SQLITE_OK)
		ERR_PRINT(" sqlite3_config(SQLITE_CONFIG_MULTITHREAD)");

	const auto initialize_result = sqlite3_initialize();
	if (initialize_result != SQLITE_OK)
		ERR_PRINT(" sqlite3_initialize()");

	storage = std::make_unique<Storage>(create_storage("C:\\ws\\Godot\\godot\\bin\\editor-database.sqlite"));
	storage->limit.worker_threads(16);
	storage->pragma.journal_mode(sqlite_orm::journal_mode::MEMORY);

	WARN_PRINT("storage->limit.worker_threads: " + itos(storage->limit.worker_threads()));
	WARN_PRINT("storage->limit.expr_depth: " + itos(storage->limit.expr_depth()));
	WARN_PRINT("storage->limit.vdbe_op: " + itos(storage->limit.vdbe_op()));
	WARN_PRINT("storage->limit.function_arg: " + itos(storage->limit.function_arg()));
	WARN_PRINT("storage->limit.sql_length: " + itos(storage->limit.sql_length()));
		  
	auto syncSchemaRes = storage->sync_schema();
	for (auto &p : syncSchemaRes) {
		const auto state = p.second;
		WARN_PRINT("EditorFileSystemDb: " + String(p.first.c_str()) + String(" ") + itos(static_cast<int64_t>(state)));
	}

	WARN_PRINT("storage->is_opened #1: " + itos(storage->is_opened()));
	storage->open_forever();
	WARN_PRINT("storage->is_opened #2: " + itos(storage->is_opened()));

	WARN_PRINT("storage->vacuum started");
	storage->vacuum();
	WARN_PRINT("storage->vacuum finished");
}

std::optional<EditorAsset> EditorFileSystemDb::asset_get(const String &p_path) const {
	auto entry = storage->get_all<EditorAsset>(sqlite_orm::where(sqlite_orm::eq(&EditorAsset::file_path, p_path)), sqlite_orm::limit(1));
	if (entry.empty())
		return std::optional<EditorAsset>();
	return std::optional(entry.at(0));
}

std::vector<EditorAssetParam> EditorFileSystemDb::asset_params_get(const int32_t &asset_id) const {
	auto entry = storage->get_all<EditorAssetParam>(sqlite_orm::where(sqlite_orm::eq(&EditorAssetParam::asset_id, asset_id)));
	return entry;
}

std::vector<EditorAssetFile> EditorFileSystemDb::asset_files_get(const int32_t &asset_id) const {
	auto entry = storage->get_all<EditorAssetFile>(sqlite_orm::where(sqlite_orm::eq(&EditorAssetFile::asset_id, asset_id)));
	return entry;
}

void EditorFileSystemDb::asset_files_replace(const int32_t &asset_id, std::vector<EditorAssetFile> &files) const {
	storage->remove_all<EditorAssetFile>(sqlite_orm::where(sqlite_orm::eq(&EditorAssetFile::asset_id, asset_id)));
	for (auto &file : files) {
		file.asset_id = asset_id;
		storage->insert(file);
	}
}

void EditorFileSystemDb::asset_params_replace(const int32_t &asset_id, std::vector<EditorAssetParam> &params) const {
	storage->remove_all<EditorAssetParam>(sqlite_orm::where(sqlite_orm::eq(&EditorAssetParam::asset_id, asset_id)));
	for (auto &param : params) {
		param.asset_id = asset_id;
		storage->insert(param);
	}
}

void EditorFileSystemDb::asset_add_or_update(EditorAsset &asset) const {
	if (asset.has_value()) {
		storage->update(asset);
	} else {
		storage->insert(asset);
		const auto rows = storage->select(sqlite_orm::columns(&EditorAsset::asset_id), sqlite_orm::from<EditorAsset>(), sqlite_orm::where(sqlite_orm::is_equal(&EditorAsset::uid, asset.uid)));
		asset.asset_id = rows[0]._Myfirst._Val;
	}
}
