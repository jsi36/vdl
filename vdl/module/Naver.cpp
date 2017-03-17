#include <stdafx.h>

bool Naver::Run()
{
	using namespace VDLDefault;

	auto r = LoadPage(this->url).text;
	cout << endl << print("������ �ҷ����� ��...") << endl;

	auto param = Parse(r, NAVER_MID);
	cout << endl << print("Media id ���� ��...") << endl;

	string m_id = param[1], key = param[2];

	auto pInfo = LoadPage(URL_INTERNAL_NAVER + m_id,
		cpr::Parameters{{ "key",key }}).text;

	cout << print("(1/2) ���� ���� �ҷ����� ��...") << endl;

	UnicodeString title_u = title_u.fromUTF8(ParseJson(pInfo)["meta"]["subject"].asString());
	wstring title = title_u.getBuffer();
	decodeHtmlEntity(title);

	Json::Value json_list = ParseJson(pInfo)["videos"]["list"];
	int json_last_list = json_list.size() - 1;
	string *json_url = new string[json_list.size()];
	string *json_res = new string[json_list.size()];
	unsigned long *json_fsz = new unsigned long[json_list.size()];

	for (size_t i = 0; i < json_list.size(); ++i) {
		json_url[i] = json_list[i]["source"].asString();
		json_res[i] = json_list[i]["encodingOption"]["name"].asString();
		json_fsz[i] = stoul(json_list[i]["size"].asString());
	}

	cout << print("(2/2) ���� �����޴� ��...") << endl << endl
		<< "���� �ػ�: " << json_res[json_last_list] << endl
		<< "���� ������: " << (float)json_fsz[json_last_list] / (1024 * 1024) << "MB" << endl;

	VDLDefault::Download(json_url[json_last_list], L"[TVCAST] " + title + L".mp4");

	cout << print("�ٿ�ε尡 �Ϸ�Ǿ����ϴ�.") << endl;

	delete[]json_url;
	delete[]json_res;

	return true;
}