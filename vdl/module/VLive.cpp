#include <stdafx.h>


bool VLive::Run() {
	using namespace VDLDefault;

	auto r = LoadPage(this->url).text;

	string fname;
	auto param = Split(Parse(r, VLIVE_VIDEO_PARAM)[1], "[\\s\\W]*,[\\s\\W]*");
	cout << endl << print("������ �ҷ����� ��...") << endl;

	string status = param[2], videoid_long = param[5], key = param[6];

	//Determine video status
	if (status == "LIVE_ON_AIR" || status == "BIG_EVENT_ON_AIR") {
		VLive_Live m(this->url);
		return m.Run();
	}
	else if (status == "VOD_ON_AIR" || status == "BIG_EVENT_INTRO") {
		
	}
	else if (status == "LIVE_END") {
		cerr << print("���̺� ����� ����Ǿ����ϴ�. ���� �ٽú��� �غ����Դϴ�.", WRN) << endl;
		return false;
	}
	else if (status == "COMING_SOON") {
		cerr << print("��� �غ����Դϴ�. ��ø� ��ٷ��ּ���.", WRN) << endl;
		return false;
	}
	else if (status == "CANCELED") {
		cerr << print("����� ����ġ �ʰ� ��ҵǾ����ϴ�.", WRN) << endl;
		return false;
	}
	else {
		cerr << print("�� �� ���� ������ �߻��Ͽ����ϴ�.", ERR) << endl;
		return false;
	}

	auto pInfo = LoadPage(URL_INTERNAL_VLIVE,
		cpr::Parameters{ { "videoId",videoid_long },
		{ "key",key },
		{ "ptc","http" },
		{ "doct","json" },
		{ "cpt","vtt" } }).text;

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
		<< "���� �ػ�: " << json_res[json_last_list]<< endl
		<< "���� ������: " << (float)json_fsz[json_last_list] / (1024 * 1024) << "MB" << endl;

	bool ret = VDLDefault::Download(json_url[json_last_list], L"[V LIVE] " + title + L".mp4");

	if (ret)
		cout << print("�ٿ�ε尡 �Ϸ�Ǿ����ϴ�.") << endl;
	else
		cout << print("�ٿ�ε尡 �Ϸ���� �ʾҽ��ϴ�.", ERR) << endl;

	delete[]json_url;
	delete[]json_res;

	return ret;
}

bool VLive_Live::Run()
{
	vector<string> v_id = Parse(this->url, VDLDefault::URL_VLIVE);
	string page_body = VDLDefault::LoadPage(this->url).text;
	auto param = Split(VLive::Parse(page_body, VDLDefault::VLIVE_VIDEO_PARAM)[1], "[\\s\\W]*,[\\s\\W]*");  //Get live status
	string status = param[2];

	cout << VDLDefault::print("(1/2) ���� ���� �ҷ����� ��...") << endl;
	UnicodeString title_u = title_u.fromUTF8(VLive::Parse(page_body, VDLDefault::VLIVE_VIDEO_TITLE)[1]);  //set title
	wstring title = title_u.getBuffer();
	VDLDefault::decodeHtmlEntity(title);

	cout << VDLDefault::print("(2/2) ���� ��ȭ�ϴ� ��...") << endl;
	for (; status != "LIVE_END";) {
		auto r_p = cpr::Post(cpr::Url{ VDLDefault::URL_INTERNAL_VLIVE_LIVE },
			cpr::Payload{ { "videoSeq",v_id[1] } },
			cpr::Header{ { "Referer",VDLDefault::URL_VLIVE + "/video" + v_id[1] },
			{ "Content-Type","application/x-www-form-urlencoded" } }).text;
		//Get 
		auto json_live_param = ParseJson(VDLDefault::replace(VDLModule::Parse(r_p, VDLDefault::VLIVE_LIVE_STREAM)[1], "\\\\\"", "\""))["resolutions"];
		string cdnUrl = json_live_param[json_live_param.size() - 1]["cdnUrl"].asString();
		string cdnChunklistUrl = VDLDefault::replace(cdnUrl, VDLDefault::VLIVE_LIVE_CONST_PLAYLIST, VDLDefault::VLIVE_LIVE_CONST_CHUNKLIST);   //get chunklist m3u8 file
		string cdnRootUrl = VDLDefault::replace(cdnUrl, VDLDefault::VLIVE_LIVE_CONST_PLAYLIST, "");  //set CDN root url
		auto chunkString = VDLDefault::LoadPage(cdnChunklistUrl).text;

		for (short i = 1; i <= 3; i++)
		{
			string url_ts = cdnRootUrl + VLive::Parse(chunkString, VDLDefault::VLIVE_LIVE_M3U8)[i*2];  //media url
			int len_ts = stoi(VLive::Parse(chunkString, VDLDefault::VLIVE_LIVE_M3U8)[(i * 2) - 1]) * 1000;  //media length
			Download(url_ts, title + L".ts");  //Download video snippet
			this_thread::sleep_for(chrono::milliseconds(len_ts - 550));  //Sleep for media length
		}

		page_body = VDLDefault::LoadPage(this->url).text;
		status = Split(VLive::Parse(page_body, VDLDefault::VLIVE_VIDEO_PARAM)[1], "[\\s\\W]*,[\\s\\W]*")[2];
	}
	this->PostProcess(title);

	return true;
}

bool VLive_Ch::Run() {
	auto body_page = VDLDefault::LoadPage(this->url);
	if (body_page.status_code != 200) {
		cerr << VDLDefault::print("Error while loading page. Check Internet connection.", ERR) << endl;
		return false;
	}

	code_channel = VDLModule::Parse(this->url, VDLDefault::VLIVE_CH_CHANNEL_CODE)[1];  //Get channel code

	string url_app_js = VDLModule::Parse(body_page.text, VDLDefault::VLIVE_CH_APP_JS)[1];  //Get app js URL
	cout << VDLDefault::print("[1/3] app ID ���� ��") << endl;
	string app_js = VDLDefault::LoadPage(url_app_js).text;  //Get app js
	app_id = VDLModule::Parse(app_js, VDLDefault::VLIVE_CH_APPID)[1];  //Set app id from app js

	vector<string> vod_url = FetchVideoList();
	string num_vod = to_string(vod_url.size() - 1);

	cout << endl << VDLDefault::print("ä�ο� ���� �ð��� �ټ� �ҿ�� �� �ֽ��ϴ�. ��� �����Ͻðڽ��ϱ�? [y/n] : ");
	 char c = cin.get();

	 while (c != 'y' || c != 'n') {
		 cout << endl << "�ٽ� �Է����ּ���.";
		 c = cin.get();
	 }

	switch (c) {
	case 'y':
		cout << VDLDefault::print("[3/3] ���� �����޴� ��") << endl;
		for (int i = 0; i < vod_url.size() - 1; i++) {
			VLive v(vod_url[i]);
			cout << VDLDefault::print(
				num_vod + "�� ���� �� " + to_string(i + 1) + " ��° ���� �����޴� ��", WRN) << endl;
			if (!v.Run())
				return false;
		}
		cout << VDLDefault::print("�ٿ�ε尡 �Ϸ�Ǿ����ϴ�.") << endl;
		return true;
		break;
	case 'n':
		cerr << VDLDefault::print("Aborted", ERR) << endl;
		return false;
		break;
	}
}