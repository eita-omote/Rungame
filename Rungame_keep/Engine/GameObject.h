// ============================================================
// ファイルの役割: コンポーネントの所有・更新・描画
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include "../Components/Component.h"

// =================================================================
// 【ゲームオブジェクトクラス】
// コンポーネントをアタッチして管理する「空の器」となるクラス
// =================================================================

// クラスの責務: コンポーネントの所有・更新・描画
class GameObject {
public:
	virtual ~GameObject() = default;

	// -------------------------------------------------------------
	// 【コンポーネントの追加】
	// template <typename T>: 追加したいコンポーネントの「型」を自由に変更可能
	// Args&&... args: コンポーネントのコンストラクタ引数をそのまま転送（完全転送）
	// -------------------------------------------------------------

	template <typename T, typename... Args>
	T* AddComponent(Args&&... args)
	{
		// std::make_unique<T>(...) でコンポーネントを生成し、unique_ptr で管理して所有者は GameObject(this) を渡す
		auto comp = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = comp.get();
		comp->Initialize(this);

		// typeid(T) をキーにして型ごとに1つのコンポーネントをマップに保存
		m_components[typeid(T)] = std::move(comp);
		return ptr;
	}

	// -------------------------------------------------------------
	// 【コンポーネントの取得】
	// 指定した型 T のコンポーネントが装着されていればそのポインタを返す
	// -------------------------------------------------------------

	template <typename T>
	T* GetComponent() const
	{
		auto it = m_components.find(typeid(T));
		if (it != m_components.end())
		{
			// 親クラス Component* から目的の派生クラス T* へ安全にキャスト
			return static_cast<T*>(it->second.get());
		}
		return nullptr;// 見つからなかった場合は nullptr を返す
	}

	//アタッチされている全コンポーネントのUpdateを順に呼び出す
	void Update(float deltaTime)
	{
		if (!m_active)return;
		for (auto& pair : m_components) {
			pair.second->Update(deltaTime);
		}
	}

	//アタッチされている全コンポーネントのDrawを順に呼び出す
	void Draw(ID3D12GraphicsCommandList* cmdList, CubeRenderer& renderer,
		const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& proj)
	{
		if (!m_active)return;
		for (auto& pair : m_components) {
			pair.second->Draw(cmdList, renderer, view, proj);
		}
	}

	// ゲームオブジェクトの有効/無効状態を取得・設定する
	bool IsActive() const { return m_active; }
	void SetActive(bool active) { m_active = active; }

private:
	bool m_active = true;	// ゲームオブジェクトの有効/無効状態

	// 型情報(std::type_index)をキーにしてコンポーネントを管理する連想配列（unordered_map）を使用
	std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;
};
