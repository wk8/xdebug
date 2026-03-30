#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shims.h"
#include "phuck_off.c"

typedef struct {
	const char *path;
	int line_no;
	int expected_id;
} positive_case;

typedef struct {
	const char *name;
	const char *fixture_path;
	const char *expected_root;
	positive_case positive_cases[20];
	size_t positive_case_count;
	const char *ignored_path;
	const char *missing_in_root_path;
	const char *prefix_only_path;
	const char *outside_root_path;
} fixture_case;

static const fixture_case fixture_cases[] = {
	{
		"api",
		"/Users/wk/pushpress/xdebug/phuck_off_tests/fixtures/api.txt",
		"/Users/wk/pushpress/pps-clean/api",
		{
			{"/Users/wk/pushpress/pps-clean/api/v1/app/library/micro/messages/Auth.php", 40, 1},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/library/application/Micro.php", 207, 190},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/library/services/microprocess/checkin.php", 18, 438},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/library/services/Checkin.php", 2178, 466},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/TrackWorkoutTypes.php", 48, 535},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/SchedulerTypes.php", 102, 894},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/SchedulerCredits.php", 170, 939},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/OrderData.php", 127, 1235},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/Users.php", 110, 1366},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/Notifications.php", 91, 1456},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/Checkins.php", 240, 1473},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/PreorderSales.php", 123, 1589},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/ProductInventoryLogs.php", 78, 1653},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/TransactionLog.php", 71, 1738},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/Products.php", 339, 1781},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/models/CalendarSessionTypes.php", 15, 2073},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/controllers/MetricsCoachController.php", 184, 2139},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/controllers/HookController.php", 243, 2181},
			{"/Users/wk/pushpress/pps-clean/api/v1/app/controllers/TaxRateController.php", 16, 2490},
			{"/Users/wk/pushpress/pps-clean/api/console/index.php", 12, 2675}
		},
		20,
		"/Users/wk/pushpress/pps-clean/api/private/rector_downgrade_to_php5_config.php",
		"/Users/wk/pushpress/pps-clean/api/definitely-not-in-dump.php",
		"/Users/wk/pushpress/pps-clean/api-shadow/definitely-not-in-dump.php",
		"/tmp/phuck-off-outside-api.php"
	},
	{
		"control-panel",
		"/Users/wk/pushpress/xdebug/phuck_off_tests/fixtures/control-panel.txt",
		"/Users/wk/pushpress/pps-clean/control-panel-legacy",
		{
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/writeable/UploadHandler.php", 42, 1},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/writeable/UploadHandler.php", 477, 20},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/libraries/oAuth/facebook/base_facebook.php", 667, 105},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/libraries/opengateway.php", 31, 352},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/libraries/opengateway.php", 453, 368},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/libraries/services/CalendarService.php", 50, 547},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/libraries/oauth2-client/test/src/Provider/GithubTest.php", 20, 622},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/models/plan_model.php", 1806, 944},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/models/charge_data_model.php", 118, 1063},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/models/customer_billing_model.php", 369, 1443},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/models/customer_billing_model.php", 771, 1452},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/controllers/reports.php", 712, 1714},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/controllers/reports.php", 1560, 1721},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/controllers/reports.php", 2003, 1724},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/controllers/members.php", 7950, 1903},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/controllers/advisor.php", 21, 1928},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/controllers/console.php", 134, 2197},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/controllers/pushpress.php", 736, 2275},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/application/controllers/UploadHandler.php", 1346, 2531},
			{"/Users/wk/pushpress/pps-clean/control-panel-legacy/assets/json/pp_json_import.php", 84, 2696}
		},
		20,
		"/Users/wk/pushpress/pps-clean/control-panel-legacy/branding/bootstrap/fullcalendar-2.7.2/demos/php/get-timezones.php",
		"/Users/wk/pushpress/pps-clean/control-panel-legacy/definitely-not-in-dump.php",
		"/Users/wk/pushpress/pps-clean/control-panel-legacy-shadow/definitely-not-in-dump.php",
		"/tmp/phuck-off-outside-control-panel.php"
	}
};

static int failures = 0;
static char backup_template[] = "/tmp/phuck-off.parser-lookup.backup.XXXXXX";
static int backup_exists = 0;
static char *saved_env = NULL;
static int env_was_set = 0;

static void assert_true(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "%s\n", message);
		failures = 1;
	}
}

static void assert_contains(const char *haystack, const char *needle, const char *message)
{
	assert_true(haystack != NULL && strstr(haystack, needle) != NULL, message);
}

static void assert_not_contains(const char *haystack, const char *needle, const char *message)
{
	assert_true(haystack == NULL || strstr(haystack, needle) == NULL, message);
}

static char *dup_string(const char *value)
{
	size_t len;
	char *copy;

	if (!value) {
		return NULL;
	}

	len = strlen(value);
	copy = (char *) malloc(len + 1);
	assert_true(copy != NULL, "failed to allocate env copy");
	if (!copy) {
		return NULL;
	}

	memcpy(copy, value, len + 1);
	return copy;
}

static void preserve_environment(void)
{
	const char *value = getenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);

	if (value) {
		saved_env = dup_string(value);
		env_was_set = 1;
	} else {
		saved_env = NULL;
		env_was_set = 0;
	}
}

static void restore_environment(void)
{
	if (env_was_set) {
		setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, saved_env, 1);
	} else {
		unsetenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR);
	}

	free(saved_env);
	saved_env = NULL;
	env_was_set = 0;
}

static void backup_existing_log(void)
{
	int fd;

	if (access(PHUCK_OFF_LOG_FILE, F_OK) != 0) {
		backup_exists = 0;
		return;
	}

	fd = mkstemp(backup_template);
	assert_true(fd >= 0, "failed to create backup path for parser lookup log");
	if (fd < 0) {
		return;
	}

	close(fd);
	unlink(backup_template);

	assert_true(rename(PHUCK_OFF_LOG_FILE, backup_template) == 0, "failed to back up parser lookup log");
	if (!failures) {
		backup_exists = 1;
	}
}

static void restore_existing_log(void)
{
	phuck_off_shutdown();
	if (unlink(PHUCK_OFF_LOG_FILE) != 0 && errno != ENOENT) {
		assert_true(0, "failed to remove parser lookup log");
	}

	if (backup_exists) {
		assert_true(rename(backup_template, PHUCK_OFF_LOG_FILE) == 0, "failed to restore original parser lookup log");
		backup_exists = 0;
	}
}

static void remove_test_log(void)
{
	phuck_off_shutdown();
	if (unlink(PHUCK_OFF_LOG_FILE) != 0 && errno != ENOENT) {
		assert_true(0, "failed to remove parser lookup test log");
	}
}

static char *read_log_file(void)
{
	FILE *fp;
	long length;
	size_t read_bytes;
	char *buffer;

	fp = fopen(PHUCK_OFF_LOG_FILE, "rb");
	if (!fp) {
		return NULL;
	}

	assert_true(fseek(fp, 0, SEEK_END) == 0, "failed to seek parser lookup log");
	length = ftell(fp);
	assert_true(length >= 0, "failed to get parser lookup log length");
	assert_true(fseek(fp, 0, SEEK_SET) == 0, "failed to rewind parser lookup log");
	if (failures) {
		fclose(fp);
		return NULL;
	}

	buffer = (char *) malloc((size_t) length + 1);
	assert_true(buffer != NULL, "failed to allocate parser lookup log buffer");
	if (!buffer) {
		fclose(fp);
		return NULL;
	}

	read_bytes = fread(buffer, 1, (size_t) length, fp);
	assert_true(read_bytes == (size_t) length, "failed to read parser lookup log");
	buffer[read_bytes] = '\0';
	fclose(fp);
	return buffer;
}

static int init_handler_from_file(const char *path)
{
	char error[512];

	shutdown_handler();

	if (!phuck_off_parse_funcs_file(path, &handler.files, &handler.user_code_root, error, sizeof(error))) {
		fprintf(stderr, "failed to initialize handler from %s: %s\n", path, error);
		failures = 1;
		handler.initialized = 0;
		return 0;
	}

	handler.user_code_root_len = strlen(handler.user_code_root);
	handler.initialized = 1;
	return 1;
}

static void assert_expected_function_id(const char *path, int line_no, int expected_id)
{
	int id = function_id(path, line_no);
	if (id != expected_id) {
		fprintf(stderr, "expected function id %d for %s:%d, got %d\n", expected_id, path, line_no, id);
		failures = 1;
	}
}

static void assert_missing_function_id(const char *path, int line_no)
{
	int id = function_id(path, line_no);
	if (id != -1) {
		fprintf(stderr, "expected missing function id for %s:%d, got %d\n", path, line_no, id);
		failures = 1;
	}
}

static void run_fixture_case(const fixture_case *fixture)
{
	size_t i;
	const positive_case *first_positive = &fixture->positive_cases[0];
	char message[256];
	char expected_missing_line[512];
	char expected_line_zero[512];
	char expected_missing_file[512];
	char *log_content;

	remove_test_log();
	assert_true(setenv(PHUCK_OFF_LOG_LEVEL_ENV_VAR, "error", 1) == 0, "failed to set parser lookup log level");
	phuck_off_logger_init();
	if (!init_handler_from_file(fixture->fixture_path)) {
		phuck_off_shutdown();
		return;
	}

	snprintf(message, sizeof(message), "%s handler failed to initialize", fixture->name);
	assert_true(handler.initialized == 1, message);
	snprintf(message, sizeof(message), "%s handler files hash missing", fixture->name);
	assert_true(handler.files != NULL, message);
	snprintf(message, sizeof(message), "unexpected user_code_root for %s", fixture->name);
	assert_true(strcmp(handler.user_code_root, fixture->expected_root) == 0, message);

	for (i = 0; i < fixture->positive_case_count; i++) {
		assert_expected_function_id(
			fixture->positive_cases[i].path,
			fixture->positive_cases[i].line_no,
			fixture->positive_cases[i].expected_id
		);
	}

	assert_missing_function_id(first_positive->path, 999999);
	assert_missing_function_id(first_positive->path, 0);
	assert_missing_function_id(fixture->ignored_path, 1);
	assert_missing_function_id(fixture->ignored_path, 2);
	assert_missing_function_id(fixture->missing_in_root_path, 1);
	assert_missing_function_id(fixture->prefix_only_path, 1);
	assert_missing_function_id(fixture->outside_root_path, 1);

	phuck_off_shutdown();
	log_content = read_log_file();
	snprintf(message, sizeof(message), "%s should produce an error log file", fixture->name);
	assert_true(log_content != NULL, message);

	snprintf(expected_missing_line, sizeof(expected_missing_line), "No function id entry for \"%s\":999999", first_positive->path);
	snprintf(expected_line_zero, sizeof(expected_line_zero), "No function id entry for \"%s\":0", first_positive->path);
	snprintf(expected_missing_file, sizeof(expected_missing_file), "No function map entry for path \"%s\"", fixture->missing_in_root_path);

	snprintf(message, sizeof(message), "missing-line error log missing for %s", fixture->name);
	assert_contains(log_content, expected_missing_line, message);
	snprintf(message, sizeof(message), "line-zero error log missing for %s", fixture->name);
	assert_contains(log_content, expected_line_zero, message);
	snprintf(message, sizeof(message), "missing-file error log missing for %s", fixture->name);
	assert_contains(log_content, expected_missing_file, message);
	snprintf(message, sizeof(message), "ignored path should not emit error log for %s", fixture->name);
	assert_not_contains(log_content, fixture->ignored_path, message);
	snprintf(message, sizeof(message), "prefix-only path should not emit error log for %s", fixture->name);
	assert_not_contains(log_content, fixture->prefix_only_path, message);
	snprintf(message, sizeof(message), "outside-root path should not emit error log for %s", fixture->name);
	assert_not_contains(log_content, fixture->outside_root_path, message);

	free(log_content);
}

int main(void)
{
	size_t i;

	preserve_environment();
	backup_existing_log();

	for (i = 0; i < sizeof(fixture_cases) / sizeof(fixture_cases[0]); i++) {
		run_fixture_case(&fixture_cases[i]);
	}

	restore_existing_log();
	restore_environment();

	if (failures) {
		return 1;
	}

	printf("ok\n");
	return 0;
}
