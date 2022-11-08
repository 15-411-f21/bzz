#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "GL.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <unordered_set>

int PlayMode::Cricket::seq = 0;

GLuint bzz_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > bzz_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("bzz.pnct"));
	bzz_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > bzz_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("bzz.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = bzz_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = bzz_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

// https://stackoverflow.com/questions/5289613/generate-random-float-between-two-floats
float get_rng_range(float a, float b) {
	float random = ((float) rand()) / (float) RAND_MAX;
  float diff = b - a;
  float r = random * diff;
  return a + r;
}

Load< Sound::Sample > chirping_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("chirping.wav"));
});

Load< Sound::Sample > click_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("click.wav"));
});

Load< Sound::Sample > cash_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("cash.wav"));
});

Load< Sound::Sample > background_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("background_music.wav"));
});

Scene::Transform* PlayMode::spawn_strawberry() {

	Mesh const &mesh = bzz_meshes->lookup("Strawberry");

	scene.transforms.emplace_back();

	Scene::Transform *transform = &scene.transforms.back();

	const float eps = 0.2f;
	transform->position = glm::vec3(get_rng_range(bedding_min.x + eps, bedding_max.x - eps), get_rng_range(bedding_min.y + eps, bedding_max.y - eps), strawberry_transform->position.z);
	transform->rotation = glm::angleAxis(glm::radians(get_rng_range(0.f,360.f)), glm::vec3(0.0,0.0,1.0));
	transform->scale = glm::vec3(1.f);
	transform->name = "Strawberry";

	scene.drawables.emplace_back(Scene::Drawable(transform));
	Scene::Drawable &drawable = scene.drawables.back();

	drawable.pipeline = lit_color_texture_program_pipeline;

	drawable.pipeline.vao = bzz_meshes_for_lit_color_texture_program;
	drawable.pipeline.type = mesh.type;
	drawable.pipeline.start = mesh.start;
	drawable.pipeline.count = mesh.count;

	return transform;
}

void PlayMode::spawn_cricket() {

	Mesh const &mesh = bzz_meshes->lookup("BabyCricket");

	scene.transforms.emplace_back();

	Scene::Transform *transform = &scene.transforms.back();
	Cricket cricket = Cricket(Cricket::seq++, transform);
	Crickets.push_back(cricket);

	transform->position = glm::vec3(get_rng_range(bedding_min.x,bedding_max.x), get_rng_range(bedding_min.y,bedding_max.y), baby_cricket_transform->position.z);
	transform->rotation = glm::angleAxis(glm::radians(get_rng_range(0.f,360.f)), glm::vec3(0.0,0.0,1.0));
	transform->scale = glm::vec3(1.f);
	transform->name = "Cricket_" + std::to_string(cricket.cricketID);

	scene.drawables.emplace_back(Scene::Drawable(transform));
	Scene::Drawable &drawable = scene.drawables.back();

	drawable.pipeline = lit_color_texture_program_pipeline;

	drawable.pipeline.vao = bzz_meshes_for_lit_color_texture_program;
	drawable.pipeline.type = mesh.type;
	drawable.pipeline.start = mesh.start;
	drawable.pipeline.count = mesh.count;

}

void PlayMode::kill_cricket(Cricket &cricket) {
	glm::vec3 axis = glm::normalize(glm::vec3(0.f, 1.f, 0.f));
	glm:: quat turn_upside_down = glm::angleAxis(glm::radians(180.f), axis);
	cricket.transform->rotation = glm::normalize(cricket.transform->rotation * turn_upside_down);
}


PlayMode::PlayMode() : scene(*bzz_scene), game_UI(this) {

	for (auto &transform : scene.transforms) {
		if (transform.name == "AdultCricket") {
			adult_cricket_transform = &transform;
			adult_cricket_transform->scale = glm::vec3(0.f);
		}
		if (transform.name == "BabyCricket") {
			baby_cricket_transform = &transform;
			baby_cricket_transform->scale = glm::vec3(0.f);
		}
		if (transform.name == "Strawberry") {
			strawberry_transform = &transform;
			strawberry_transform->scale = glm::vec3(0.f);
		}
		if (transform.name == "Bedding") {
			bedding_transform = &transform;
		}
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// Loop chirping sound and background music
	Sound::loop(*chirping_sample, 1.0f, 0.0f);
	Sound::loop(*background_sample, 1.0f, 0.0f);

	Mesh const &mesh = bzz_meshes->lookup("Bedding");
	glm::mat4x3 to_world = bedding_transform->make_local_to_world();
	bedding_min = to_world * glm::vec4(mesh.min, 1.f);
	bedding_max = to_world * glm::vec4(mesh.max, 1.f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		std::cout << x << ", " << y << std::endl;
		game_UI.update(x, y, evt.type == SDL_MOUSEBUTTONDOWN);
		return true;
	} 

	return false;
}

void PlayMode::update(float elapsed) {

	// update food visuals
	{
		int n_strawberries = int((totalFood + 199.f) / 200.f);
		while(strawberry_transforms.size() > n_strawberries) {
			strawberry_transforms.pop_front();
		}
		while(strawberry_transforms.size() < n_strawberries) {
			strawberry_transforms.push_back(spawn_strawberry());
		}
		auto it = scene.drawables.begin();
		while(it != scene.drawables.end()) {
			Scene::Drawable dr = *it;
			if ( it->transform->name == "Strawberry" && std::find(strawberry_transforms.begin(), strawberry_transforms.end(), it->transform) == strawberry_transforms.end() ) {
				it = scene.drawables.erase(it);
			} else {
				it++;
			}
		}
	}

	// update crickets age and state
	{
		for(Cricket &cricket: Crickets) {
			if (cricket.is_dead()) {
				continue;
			}
			cricket.age += elapsed;
			if(cricket.is_dead()) {
				kill_cricket(cricket);
			} else if (cricket.is_mature()) {
				mature_cricket(cricket);
			}
		}
	}

	// Move some crickets randomly
	{
		const float JumpDistance = 0.1f;
		for(Cricket &cricket: Crickets) {
			if(cricket.is_dead()) {
				continue;
			}
			if(std::rand() % 40 == 0) {

				glm::quat quat = glm::angleAxis(glm::radians(get_rng_range(-20.f,20.f)), glm::vec3(0.0,0.0,1.0));
				cricket.transform->rotation = glm::normalize(cricket.transform->rotation * quat);

				glm::vec3 dir = cricket.transform->rotation * glm::vec3(0.f, 1.f, 0.f) ;
				dir = JumpDistance * glm::normalize(dir);
				cricket.transform->position += dir;

				glm::vec3 &pos = cricket.transform->position;
				const float eps = 0.2f;

				if (!(bedding_min.x + eps < pos.x && pos.x < bedding_max.x - eps && bedding_min.y + eps < pos.y && pos.y < bedding_max.y - eps)) {
					glm::quat turn_around = glm::angleAxis(glm::radians(180.f), glm::vec3(0.0,0.0,1.0));
					cricket.transform->rotation = glm::normalize(cricket.transform->rotation * turn_around);

					pos.x = glm::clamp(pos.x, bedding_min.x + eps, bedding_max.x - eps);
					pos.y = glm::clamp(pos.y, bedding_min.y + eps, bedding_max.y - eps);
				}
			}
		}
	}

	//update counts
	{
		numBabyCrickets = 0;
		numMatureCrickets = 0;
		numDeadCrickets = 0;
		for(Cricket &cricket: Crickets) {
			if (cricket.is_mature()){
				numMatureCrickets ++;
			}
			if (cricket.is_dead()) {
				numDeadCrickets++;
			}
			if(cricket.is_baby()) {
				numBabyCrickets++;
			}
		}
		assert(numBabyCrickets + numMatureCrickets + numDeadCrickets == Crickets.size());
	}

	//update food and starvation
	{
		// Todo: do adults eat more than babies ?
		// Todo: do not eat at every frame
		if (totalFood == 0.f){
			auto is_starving = [](){
				return rand() % 1000 + 1 < 6;
			};
			for( Cricket &cricket: Crickets) {
				if(cricket.is_dead()) {
					continue;
				}
				cricket.starved = is_starving();
				if(cricket.is_dead()) {
					kill_cricket(cricket);
				}
			}
		}
		totalFood = std::max(0.f, totalFood - (numBabyCrickets + numMatureCrickets) * cricketEatingRate);
	}

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 10.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		camera->transform->position.x += move.x;
		camera->transform->position.y += move.y;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;

		game_UI.draw();

		lines.draw_text("Food: " + std::to_string((int)totalFood),
			glm::vec3(aspect - 0.8f, 1.0f - 0.30f, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));

		lines.draw_text("Money: " + std::to_string((int)totalMoney),
			glm::vec3(aspect - 0.8f, 1.0f - 0.45f, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));

		lines.draw_text("Baby Crickets: " + std::to_string(numBabyCrickets),
			glm::vec3(aspect - 0.8f, 1.0f - 0.60f, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));

		lines.draw_text("Mature Crickets: " + std::to_string(numMatureCrickets),
			glm::vec3(aspect - 0.8f, 1.0f - 0.75f, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));


		lines.draw_text("WASD moves the camera",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("WASD moves the camera",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
	GL_ERRORS();
}

void PlayMode::invoke_callback(Button_UI::call_back callback) {
	Sound::play(*click_sample, 100.0f, 0.0f);
	switch(callback) {
		case Button_UI::BUY_FOOD:
			buy_food();
			break;
		case Button_UI::BUY_EGG:
			buy_eggs();
			break;
		case Button_UI::SELL_MATURE:
			Sound::play(*cash_sample, 1.0f, 0.0f);
			sell_mature();
			break;
		default:
			throw std::runtime_error("unrecognized callback\n");
	}
}

template<typename T>
void buy(T quantity, float price, T &total_quantity, float &total_money) {
	if (total_money >= price) {
		total_money -= price;
		total_quantity += quantity;
	}
}

void PlayMode::buy_food() {
	std::cout << "buy_food" << std::endl;
	const float unitFood = 200;
	const float unitPrice = 10;

	buy<float>(unitFood, unitPrice, totalFood, totalMoney);
}

void PlayMode::buy_eggs() {
	std::cout << "buy_eggs" << std::endl;
	const size_t unitEggs = 10;
	const float unitPrice = 200;

	if (totalMoney >= unitPrice) {
		for (size_t i = 0; i < unitEggs; i++)
			spawn_cricket();
		totalMoney -= unitPrice;
	}
}

void PlayMode::mature_cricket(Cricket &cricket) {
	Mesh const &mesh = bzz_meshes->lookup("AdultCricket");
	for(Scene::Drawable &drawable: scene.drawables) {
		if(drawable.transform->name == "Cricket_" + std::to_string(cricket.cricketID)) {
			drawable.pipeline.type = mesh.type;
			drawable.pipeline.start = mesh.start;
			drawable.pipeline.count = mesh.count;
			break;
		}
	}
}

void PlayMode::sell_mature() {
	std::cout << "sell_mature" << std::endl;
	const float price = 20;

	std::unordered_set<std::string> mature_cricket_names;
	std::vector<Cricket> mature_crickets;
	std::vector<Cricket> non_mature_crickets;

	for(Cricket &cricket: Crickets) {
		if(cricket.is_mature()) {
			mature_crickets.push_back(cricket);
			mature_cricket_names.insert("Cricket_" + std::to_string(cricket.cricketID));
		}else{
			non_mature_crickets.push_back(cricket);
		}
	}

	assert(mature_crickets.size() == numMatureCrickets);
	
	totalMoney += numMatureCrickets * price;
	numMatureCrickets = 0;
	Crickets = std::move(non_mature_crickets);

	auto it = scene.drawables.begin();
	while(it != scene.drawables.end()) {
		Scene::Drawable dr = *it;
		if ( mature_cricket_names.count(dr.transform->name) ) {
			it = scene.drawables.erase(it);
		} else {
			it++;
		}
	}
}